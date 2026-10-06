#include "analysis/PolyphonicNoteDetector.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace pmx::analysis
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr float absoluteAmplitudeFloor = 0.0035f;
constexpr float relativeAmplitudeFloor = 0.16f;
constexpr float localPeakRatio = 1.08f;
constexpr int evidenceRequired = 2;
constexpr float harmonicToleranceCents = 22.0f;

int midiForFrequency(double frequency) noexcept
{
    if (!std::isfinite(frequency) || frequency <= 0.0) return -1;
    return static_cast<int>(std::lround(69.0 + 12.0 * std::log2(frequency / 440.0)));
}

float centsBetween(double a,double b) noexcept
{
    if (!std::isfinite(a) || !std::isfinite(b) || a <= 0.0 || b <= 0.0)
        return std::numeric_limits<float>::infinity();
    return static_cast<float>(1200.0 * std::log2(a/b));
}
}

PolyphonicNoteDetector::PolyphonicNoteDetector() noexcept
{
    setProfile(instruments::GuitarProfile::standard());
    reset();
}

void PolyphonicNoteDetector::prepare(double sampleRate) noexcept
{
    sampleRateHz = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 44100.0;

    analysisWindowScale=0.0;
    for(int i=0;i<windowSize;++i)
    {
        const auto phase=2.0*pi*static_cast<double>(i)/static_cast<double>(windowSize-1);
        const auto value=static_cast<float>(0.5-0.5*std::cos(phase));
        analysisWindow[static_cast<std::size_t>(i)]=value;
        analysisWindowScale+=value;
    }
    if(analysisWindowScale<=0.0)analysisWindowScale=1.0;

    reset();
}

void PolyphonicNoteDetector::reset() noexcept
{
    history.fill(0.0f);
    writePosition=0;
    historyCount=0;
    samplesSinceAnalysis=0;
    evidence.fill(0);
    localProfileGeneration=profileGeneration.load(std::memory_order_acquire);
    publishEmpty();
}

void PolyphonicNoteDetector::setProfile(const instruments::GuitarProfile& profile) noexcept
{
    const auto safe=profile.valid()?profile:instruments::GuitarProfile::standard();

    std::array<double,128> sums{};
    std::array<int,128> counts{};

    for(std::size_t stringIndex=0;stringIndex<instruments::GuitarProfile::stringCount;++stringIndex)
    {
        for(int fret=0;fret<=instruments::GuitarProfile::maxFret;++fret)
        {
            const auto frequency=safe.fretFrequency(stringIndex,fret);
            const int midi=midiForFrequency(frequency);
            if(midi<0||midi>=128||!std::isfinite(frequency)||frequency<=0.0)continue;
            sums[static_cast<std::size_t>(midi)]+=frequency;
            ++counts[static_cast<std::size_t>(midi)];
        }
    }

    int out=0;
    for(int midi=0;midi<128 && out<maxCandidates;++midi)
    {
        const int count=counts[static_cast<std::size_t>(midi)];
        if(count<=0)continue;
        candidateMidi[static_cast<std::size_t>(out)].store(midi,std::memory_order_relaxed);
        candidateFrequency[static_cast<std::size_t>(out)].store(
            sums[static_cast<std::size_t>(midi)]/static_cast<double>(count),
            std::memory_order_relaxed);
        ++out;
    }

    candidateCount.store(out,std::memory_order_release);
    profileGeneration.fetch_add(1,std::memory_order_acq_rel);
}

void PolyphonicNoteDetector::process(const float* samples,int numSamples) noexcept
{
    if(numSamples<=0)return;

    for(int i=0;i<numSamples;++i)
    {
        float value=samples?samples[i]:0.0f;
        if(!std::isfinite(value))value=0.0f;

        history[static_cast<std::size_t>(writePosition)]=value;
        writePosition=(writePosition+1)%windowSize;
        historyCount=std::min(historyCount+1,windowSize);

        if(++samplesSinceAnalysis>=analysisHop)
        {
            samplesSinceAnalysis=0;
            if(historyCount>=windowSize)analyse();
        }
    }
}

void PolyphonicNoteDetector::rebuildCandidateStateIfNeeded() noexcept
{
    const auto generation=profileGeneration.load(std::memory_order_acquire);
    if(generation==localProfileGeneration)return;

    evidence.fill(0);
    localProfileGeneration=generation;
    publishEmpty();
}

float PolyphonicNoteDetector::spectralAmplitude(double frequency) const noexcept
{
    if(!std::isfinite(frequency)||frequency<=0.0||frequency>=sampleRateHz*0.48)return 0.0f;

    const double omega=2.0*pi*frequency/sampleRateHz;
    const double stepCos=std::cos(omega);
    const double stepSin=std::sin(omega);
    double oscillatorCos=1.0;
    double oscillatorSin=0.0;
    double real=0.0;
    double imag=0.0;

    const int oldest=(writePosition-historyCount+windowSize)%windowSize;
    for(int i=0;i<historyCount;++i)
    {
        const auto sample=static_cast<double>(history[static_cast<std::size_t>((oldest+i)%windowSize)])
                         * static_cast<double>(analysisWindow[static_cast<std::size_t>(i)]);
        real+=sample*oscillatorCos;
        imag-=sample*oscillatorSin;

        const double nextCos=oscillatorCos*stepCos-oscillatorSin*stepSin;
        oscillatorSin=oscillatorSin*stepCos+oscillatorCos*stepSin;
        oscillatorCos=nextCos;
    }

    return static_cast<float>(2.0*std::sqrt(real*real+imag*imag)/analysisWindowScale);
}

bool PolyphonicNoteDetector::explainedBySelectedHarmonic(
    const Score& candidate,
    const std::array<Score,PolyphonicSnapshot::maxNotes>& selected,
    int selectedCount) const noexcept
{
    for(int i=0;i<selectedCount;++i)
    {
        const auto& fundamental=selected[static_cast<std::size_t>(i)];
        if(fundamental.frequency<=0.0||candidate.frequency<=fundamental.frequency)continue;

        const double ratio=candidate.frequency/fundamental.frequency;
        for(int harmonic=2;harmonic<=5;++harmonic)
        {
            const auto harmonicFrequency=fundamental.frequency*static_cast<double>(harmonic);
            if(std::abs(centsBetween(candidate.frequency,harmonicFrequency))>harmonicToleranceCents)continue;

            // Real GuitarSet pickup recordings frequently have an octave
            // overtone that is as strong as (or slightly stronger than) the
            // played fundamental. Be more conservative about promoting those
            // harmonics to independent notes.
            const float allowance=harmonic==2?1.10f:(harmonic==3?0.72f:0.58f);
            if(candidate.amplitude<=fundamental.amplitude*allowance)
                return true;
        }
    }
    return false;
}

void PolyphonicNoteDetector::analyse() noexcept
{
    rebuildCandidateStateIfNeeded();

    const int count=candidateCount.load(std::memory_order_acquire);
    if(count<=0)
    {
        publishEmpty();
        return;
    }

    double sumSq=0.0;
    for(const auto sample:history)sumSq+=static_cast<double>(sample)*sample;
    const float frameLevel=static_cast<float>(std::sqrt(sumSq/static_cast<double>(windowSize)));
    if(frameLevel<0.0015f)
    {
        evidence.fill(0);
        publishEmpty(frameLevel);
        return;
    }

    std::array<Score,maxCandidates> scores{};
    float strongest=0.0f;

    for(int i=0;i<count;++i)
    {
        auto& score=scores[static_cast<std::size_t>(i)];
        score.candidateIndex=i;
        score.midi=candidateMidi[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        score.frequency=candidateFrequency[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        score.amplitude=spectralAmplitude(score.frequency);
        strongest=std::max(strongest,score.amplitude);
    }

    if(strongest<absoluteAmplitudeFloor)
    {
        evidence.fill(0);
        publishEmpty(frameLevel);
        return;
    }

    for(int i=0;i<count;++i)
    {
        auto& score=scores[static_cast<std::size_t>(i)];
        const float left=i>0?scores[static_cast<std::size_t>(i-1)].amplitude:0.0f;
        const float right=i+1<count?scores[static_cast<std::size_t>(i+1)].amplitude:0.0f;
        score.localPeak=score.amplitude>=left*localPeakRatio && score.amplitude>=right*localPeakRatio;

        score.supported=score.amplitude;
        for(int harmonic=2;harmonic<=4;++harmonic)
        {
            const double target=score.frequency*static_cast<double>(harmonic);
            float nearestAmplitude=0.0f;
            float nearestCents=std::numeric_limits<float>::infinity();

            for(int j=i+1;j<count;++j)
            {
                const auto distance=std::abs(centsBetween(scores[static_cast<std::size_t>(j)].frequency,target));
                if(distance<nearestCents)
                {
                    nearestCents=distance;
                    nearestAmplitude=scores[static_cast<std::size_t>(j)].amplitude;
                }
                if(scores[static_cast<std::size_t>(j)].frequency>target*1.04)break;
            }

            if(nearestCents<30.0f)
                score.supported+=nearestAmplitude*(harmonic==2?0.32f:(harmonic==3?0.18f:0.10f));
        }
    }

    std::array<Score,PolyphonicSnapshot::maxNotes> rawSelected{};
    int rawSelectedCount=0;
    const float threshold=std::max(absoluteAmplitudeFloor,strongest*relativeAmplitudeFloor);

    while(rawSelectedCount<PolyphonicSnapshot::maxNotes)
    {
        int best=-1;
        float bestSupported=0.0f;

        for(int i=0;i<count;++i)
        {
            auto& score=scores[static_cast<std::size_t>(i)];
            if(score.selected||!score.localPeak||score.amplitude<threshold)continue;
            if(score.supported>bestSupported)
            {
                bestSupported=score.supported;
                best=i;
            }
        }

        if(best<0)break;

        auto& candidate=scores[static_cast<std::size_t>(best)];
        candidate.selected=true;

        if(explainedBySelectedHarmonic(candidate,rawSelected,rawSelectedCount))
            continue;

        rawSelected[static_cast<std::size_t>(rawSelectedCount++)]=candidate;
    }

    std::array<bool,maxCandidates> detected{};
    for(int n=0;n<rawSelectedCount;++n)
    {
        const int index=rawSelected[static_cast<std::size_t>(n)].candidateIndex;
        if(index>=0&&index<count)detected[static_cast<std::size_t>(index)]=true;
    }

    for(int i=0;i<count;++i)
    {
        auto& value=evidence[static_cast<std::size_t>(i)];
        if(detected[static_cast<std::size_t>(i)])
            value=static_cast<unsigned char>(std::min(evidenceRequired,static_cast<int>(value)+1));
        else
            value=0;
    }

    std::array<Score,PolyphonicSnapshot::maxNotes> stable{};
    int stableCount=0;

    for(int i=0;i<count && stableCount<PolyphonicSnapshot::maxNotes;++i)
    {
        // Require consecutive support including the current analysis frame.
        // This prevents notes from lingering across fast chord/note changes.
        if(!detected[static_cast<std::size_t>(i)] ||
           evidence[static_cast<std::size_t>(i)]<evidenceRequired)continue;

        auto score=scores[static_cast<std::size_t>(i)];

        if(explainedBySelectedHarmonic(score,stable,stableCount))
            continue;

        stable[static_cast<std::size_t>(stableCount++)]=score;
    }

    // Instrument consumers prefer low-to-high note order, not spectral strength order.
    for(int i=1;i<stableCount;++i)
    {
        auto value=stable[static_cast<std::size_t>(i)];
        int j=i-1;
        while(j>=0&&stable[static_cast<std::size_t>(j)].midi>value.midi)
        {
            stable[static_cast<std::size_t>(j+1)]=stable[static_cast<std::size_t>(j)];
            --j;
        }
        stable[static_cast<std::size_t>(j+1)]=value;
    }

    publish(stable,stableCount,frameLevel);
}

void PolyphonicNoteDetector::publish(
    const std::array<Score,PolyphonicSnapshot::maxNotes>& selected,
    int selectedCount,
    float frameLevel) noexcept
{
    float strongest=0.0f;
    for(int i=0;i<selectedCount;++i)
        strongest=std::max(strongest,selected[static_cast<std::size_t>(i)].amplitude);

    for(int i=0;i<PolyphonicSnapshot::maxNotes;++i)
    {
        if(i<selectedCount)
        {
            const auto& note=selected[static_cast<std::size_t>(i)];
            const float confidence=strongest>0.0f?std::clamp(note.amplitude/strongest,0.0f,1.0f):0.0f;
            publishedMidi[static_cast<std::size_t>(i)].store(note.midi,std::memory_order_relaxed);
            publishedFrequency[static_cast<std::size_t>(i)].store(note.frequency,std::memory_order_relaxed);
            publishedConfidence[static_cast<std::size_t>(i)].store(confidence,std::memory_order_relaxed);
        }
        else
        {
            publishedMidi[static_cast<std::size_t>(i)].store(-1,std::memory_order_relaxed);
            publishedFrequency[static_cast<std::size_t>(i)].store(0.0,std::memory_order_relaxed);
            publishedConfidence[static_cast<std::size_t>(i)].store(0.0f,std::memory_order_relaxed);
        }
    }

    publishedFrameLevel.store(frameLevel,std::memory_order_relaxed);
    publishedCount.store(selectedCount,std::memory_order_release);
    publishedSerial.fetch_add(1,std::memory_order_acq_rel);
}

void PolyphonicNoteDetector::publishEmpty(float frameLevel) noexcept
{
    std::array<Score,PolyphonicSnapshot::maxNotes> empty{};
    publish(empty,0,frameLevel);
}

PolyphonicSnapshot PolyphonicNoteDetector::snapshot() const noexcept
{
    PolyphonicSnapshot result;
    result.noteCount=std::clamp(publishedCount.load(std::memory_order_acquire),0,PolyphonicSnapshot::maxNotes);
    result.frameLevel=publishedFrameLevel.load(std::memory_order_relaxed);

    for(int i=0;i<result.noteCount;++i)
    {
        auto& note=result.notes[static_cast<std::size_t>(i)];
        note.midiNote=publishedMidi[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        note.frequencyHz=publishedFrequency[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        note.confidence=publishedConfidence[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
    }

    result.serial=publishedSerial.load(std::memory_order_acquire);
    return result;
}
} // namespace pmx::analysis
