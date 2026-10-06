#include "instruments/SynthInstrumentEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::instruments
{
namespace
{
constexpr double twoPi=6.283185307179586476925286766559;
constexpr double detuneRatio=1.000404921271;
}

double SynthInstrumentEngine::midiFrequency(int midi,float cents) noexcept
{
    const auto semitones=(static_cast<double>(midi)-69.0)+(static_cast<double>(cents)/100.0);
    return 440.0*std::pow(2.0,semitones/12.0);
}

void SynthInstrumentEngine::prepare(double newSampleRate) noexcept
{
    sampleRate=std::isfinite(newSampleRate) && newSampleRate>1000.0?newSampleRate:48000.0;
    reset();
}

void SynthInstrumentEngine::reset() noexcept
{
    phase=0.0;
    detunedPhase=0.0;
    frequency=0.0;
    targetFrequency=0.0;
    envelope=0.0f;
    targetAmplitude=0.0f;
    bendCents=0.0f;
    currentMidi=-1;
    gate=false;
    publishedActive.store(false,std::memory_order_relaxed);
    publishedMidi.store(-1,std::memory_order_relaxed);
    publishedFrequency.store(0.0f,std::memory_order_relaxed);
}

void SynthInstrumentEngine::setLevel(float newLevel) noexcept
{
    if(!std::isfinite(newLevel))return;
    outputLevel.store(std::clamp(newLevel,0.0f,1.0f),std::memory_order_relaxed);
}

void SynthInstrumentEngine::setBrightness(float newBrightness) noexcept
{
    if(!std::isfinite(newBrightness))return;
    toneBrightness.store(std::clamp(newBrightness,0.0f,1.0f),std::memory_order_relaxed);
}

void SynthInstrumentEngine::handlePerformanceEvent(const performance::PerformanceEvent& event) noexcept
{
    if(event.source!=performance::PerformanceEventSource::resolved)return;
    switch(event.type)
    {
        case performance::PerformanceEventType::noteOn:
        {
            if(event.midiNote<0||event.midiNote>127)return;
            currentMidi=event.midiNote;
            bendCents=0.0f;
            const float velocity=std::isfinite(event.value)?std::clamp(event.value,0.0f,1.0f):0.0f;
            targetAmplitude=0.12f+0.24f*velocity;
            targetFrequency=midiFrequency(currentMidi,bendCents);
            if(frequency<=0.0)frequency=targetFrequency;
            gate=true;
            publishedActive.store(true,std::memory_order_relaxed);
            publishedMidi.store(currentMidi,std::memory_order_relaxed);
            publishedFrequency.store(static_cast<float>(targetFrequency),std::memory_order_relaxed);
            break;
        }
        case performance::PerformanceEventType::noteOff:
            if(event.midiNote==currentMidi)
            {
                gate=false;
                targetAmplitude=0.0f;
                publishedActive.store(false,std::memory_order_relaxed);
            }
            break;
        case performance::PerformanceEventType::pitchBend:
            if(event.midiNote==currentMidi && currentMidi>=0)
            {
                bendCents=std::isfinite(event.value)?std::clamp(event.value,-200.0f,200.0f):0.0f;
                targetFrequency=midiFrequency(currentMidi,bendCents);
                publishedFrequency.store(static_cast<float>(targetFrequency),std::memory_order_relaxed);
            }
            break;
        case performance::PerformanceEventType::chordChanged:
            break;
    }
}

void SynthInstrumentEngine::render(float* output,int numSamples) noexcept
{
    if(output==nullptr||numSamples<=0)return;
    if(!enabled.load(std::memory_order_relaxed))
    {
        std::fill_n(output,numSamples,0.0f);
        return;
    }

    const double safeRate=sampleRate>1000.0?sampleRate:48000.0;
    const float attack=static_cast<float>(1.0-std::exp(-1.0/(0.008*safeRate)));
    const float release=static_cast<float>(1.0-std::exp(-1.0/(0.120*safeRate)));
    const double glide=1.0-std::exp(-1.0/(0.006*safeRate));
    const float levelValue=outputLevel.load(std::memory_order_relaxed);
    const float bright=toneBrightness.load(std::memory_order_relaxed);

    for(int i=0;i<numSamples;++i)
    {
        frequency+=(targetFrequency-frequency)*glide;
        const float envelopeTarget=gate?targetAmplitude:0.0f;
        envelope+=(envelopeTarget-envelope)*(gate?attack:release);
        if(frequency<=0.0||envelope<0.000001f)
        {
            output[i]=0.0f;
            continue;
        }

        phase+=frequency/safeRate;
        detunedPhase+=(frequency*detuneRatio)/safeRate;
        phase-=std::floor(phase);
        detunedPhase-=std::floor(detunedPhase);

        const double sine=std::sin(twoPi*phase);
        const double detunedSine=std::sin(twoPi*detunedPhase);
        const double triangle=1.0-4.0*std::abs(phase-0.5);
        const double smooth=0.72*sine+0.28*detunedSine;
        const double brightTone=0.52*sine+0.30*triangle+0.18*detunedSine;
        const double tone=smooth*(1.0-static_cast<double>(bright))+brightTone*static_cast<double>(bright);
        output[i]=static_cast<float>(tone*static_cast<double>(envelope*levelValue));
    }
}
} // namespace pmx::instruments
