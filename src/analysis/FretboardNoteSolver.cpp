#include "analysis/FretboardNoteSolver.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace pmx::analysis
{
namespace
{
constexpr float maximumCandidateDistanceCents = 70.0f;
constexpr float transitionThresholdCents = 18.0f;
constexpr float switchThresholdCents = 65.0f;
constexpr int confirmationsToSwitch = 2;
constexpr float minimumInputConfidence = 0.03f;

int midiForFrequency(double frequency) noexcept
{
    if (!std::isfinite(frequency) || frequency <= 0.0) return -1;
    return static_cast<int>(std::lround(69.0 + 12.0 * std::log2(frequency / 440.0)));
}
}

FretboardNoteSolver::FretboardNoteSolver() noexcept
{
    setProfile(instruments::GuitarProfile::standard());
    reset();
}

void FretboardNoteSolver::reset() noexcept
{
    currentMidi = -1;
    currentTargetHz = 0.0;
    pendingMidi = -1;
    pendingCount = 0;
    lastPitchSerial = 0;
    lastOnsetSerial = 0;
    publish(false,-1,0.0,0.0,0.0f,0.0f,false,0,0);
}

void FretboardNoteSolver::setProfile(const instruments::GuitarProfile& profile) noexcept
{
    const auto safe = profile.valid() ? profile : instruments::GuitarProfile::standard();
    std::array<double, maxCandidates> generated {};
    int count = 0;

    for (std::size_t stringIndex = 0; stringIndex < instruments::GuitarProfile::stringCount; ++stringIndex)
    {
        for (int fret = 0; fret <= instruments::GuitarProfile::maxFret; ++fret)
        {
            const auto frequency = safe.fretFrequency(stringIndex,fret);
            if (std::isfinite(frequency) && frequency > 0.0 && count < maxCandidates)
                generated[static_cast<std::size_t>(count++)] = frequency;
        }
    }

    for (int i = 0; i < count; ++i)
        candidateHz[static_cast<std::size_t>(i)].store(generated[static_cast<std::size_t>(i)],std::memory_order_relaxed);

    candidateCount.store(count,std::memory_order_release);
}

float FretboardNoteSolver::centsBetween(double frequency,double target) noexcept
{
    if (!std::isfinite(frequency) || !std::isfinite(target) || frequency <= 0.0 || target <= 0.0)
        return std::numeric_limits<float>::infinity();

    return static_cast<float>(1200.0 * std::log2(frequency / target));
}

FretboardNoteSolver::Candidate FretboardNoteSolver::nearestCandidate(double frequency) const noexcept
{
    Candidate best;
    float bestDistance = std::numeric_limits<float>::infinity();

    const int count = candidateCount.load(std::memory_order_acquire);
    for (int i = 0; i < count; ++i)
    {
        const auto target = candidateHz[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        const auto cents = centsBetween(frequency,target);
        const auto distance = std::abs(cents);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best.frequency = target;
            best.midi = midiForFrequency(target);
            best.cents = cents;
            best.valid = std::isfinite(cents);
        }
    }

    if (!best.valid || bestDistance > maximumCandidateDistanceCents)
        return {};

    return best;
}

void FretboardNoteSolver::process(const MusicalAnalysisSnapshot& input) noexcept
{
    if (!input.noteActive)
    {
        currentMidi = -1;
        currentTargetHz = 0.0;
        pendingMidi = -1;
        pendingCount = 0;
        lastPitchSerial = input.pitchSerial;
        lastOnsetSerial = input.onsetSerial;
        publish(false,-1,0.0,0.0,0.0f,0.0f,false,input.onsetSerial,input.releaseSerial);
        return;
    }

    if (input.onsetSerial != lastOnsetSerial)
    {
        currentMidi = -1;
        currentTargetHz = 0.0;
        pendingMidi = -1;
        pendingCount = 0;
        lastOnsetSerial = input.onsetSerial;
    }

    if (input.pitchSerial == 0 || input.pitchSerial == lastPitchSerial)
        return;

    lastPitchSerial = input.pitchSerial;

    if (!std::isfinite(input.frequencyHz) || input.frequencyHz <= 0.0 || input.confidence < minimumInputConfidence)
    {
        publish(true,currentMidi,currentTargetHz,input.frequencyHz,0.0f,input.confidence,false,input.onsetSerial,input.releaseSerial);
        return;
    }

    const auto nearest = nearestCandidate(input.frequencyHz);
    if (!nearest.valid)
    {
        publish(true,currentMidi,currentTargetHz,input.frequencyHz,0.0f,0.0f,false,input.onsetSerial,input.releaseSerial);
        return;
    }

    if (currentMidi < 0)
    {
        currentMidi = nearest.midi;
        currentTargetHz = nearest.frequency;
        pendingMidi = -1;
        pendingCount = 0;

        const float fit = 1.0f - std::min(1.0f,std::abs(nearest.cents)/maximumCandidateDistanceCents);
        publish(true,currentMidi,currentTargetHz,input.frequencyHz,nearest.cents,
                input.confidence * (0.5f + 0.5f * fit),false,input.onsetSerial,input.releaseSerial);
        return;
    }

    const auto centsFromCurrent = centsBetween(input.frequencyHz,currentTargetHz);
    const bool moving = std::abs(centsFromCurrent) >= transitionThresholdCents;

    if (nearest.midi == currentMidi)
    {
        pendingMidi = -1;
        pendingCount = 0;
    }
    else if (std::abs(centsFromCurrent) >= switchThresholdCents)
    {
        if (pendingMidi == nearest.midi)
            ++pendingCount;
        else
        {
            pendingMidi = nearest.midi;
            pendingCount = 1;
        }

        if (pendingCount >= confirmationsToSwitch)
        {
            currentMidi = nearest.midi;
            currentTargetHz = nearest.frequency;
            pendingMidi = -1;
            pendingCount = 0;
        }
    }
    else
    {
        pendingMidi = -1;
        pendingCount = 0;
    }

    const auto resolvedCents = centsBetween(input.frequencyHz,currentTargetHz);
    const float fit = 1.0f - std::min(1.0f,std::abs(resolvedCents)/100.0f);
    publish(true,currentMidi,currentTargetHz,input.frequencyHz,resolvedCents,
            input.confidence * (0.45f + 0.55f * fit),
            moving,input.onsetSerial,input.releaseSerial);
}

void FretboardNoteSolver::publish(bool active,
                                  int midi,
                                  double target,
                                  double raw,
                                  float cents,
                                  float confidence,
                                  bool transitioning,
                                  std::uint64_t onset,
                                  std::uint64_t release) noexcept
{
    publishedActive.store(active,std::memory_order_relaxed);
    publishedMidi.store(midi,std::memory_order_relaxed);
    publishedTarget.store(target,std::memory_order_relaxed);
    publishedRaw.store(std::isfinite(raw)?raw:0.0,std::memory_order_relaxed);
    publishedCents.store(std::isfinite(cents)?cents:0.0f,std::memory_order_relaxed);
    publishedConfidence.store(std::clamp(confidence,0.0f,1.0f),std::memory_order_relaxed);
    publishedTransitioning.store(transitioning,std::memory_order_relaxed);
    publishedOnsetSerial.store(onset,std::memory_order_release);
    publishedReleaseSerial.store(release,std::memory_order_release);
}

ResolvedNoteSnapshot FretboardNoteSolver::snapshot() const noexcept
{
    ResolvedNoteSnapshot result;
    result.noteActive = publishedActive.load(std::memory_order_relaxed);
    result.midiNote = publishedMidi.load(std::memory_order_relaxed);
    result.targetFrequencyHz = publishedTarget.load(std::memory_order_relaxed);
    result.rawFrequencyHz = publishedRaw.load(std::memory_order_relaxed);
    result.centsFromTarget = publishedCents.load(std::memory_order_relaxed);
    result.confidence = publishedConfidence.load(std::memory_order_relaxed);
    result.transitioning = publishedTransitioning.load(std::memory_order_relaxed);
    result.onsetSerial = publishedOnsetSerial.load(std::memory_order_acquire);
    result.releaseSerial = publishedReleaseSerial.load(std::memory_order_acquire);
    return result;
}
} // namespace pmx::analysis
