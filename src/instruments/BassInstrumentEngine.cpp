#include "instruments/BassInstrumentEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::instruments
{
namespace { constexpr double twoPi=6.283185307179586476925286766559; }

double BassInstrumentEngine::midiFrequency(int midi,float cents) noexcept
{
    const auto semitones=(static_cast<double>(midi)-69.0)+(static_cast<double>(cents)/100.0);
    return 440.0*std::pow(2.0,semitones/12.0);
}

void BassInstrumentEngine::prepare(double newSampleRate) noexcept
{
    sampleRate=std::isfinite(newSampleRate) && newSampleRate>1000.0?newSampleRate:48000.0;
    reset();
}

void BassInstrumentEngine::reset() noexcept
{
    phase=0.0;
    frequency=0.0;
    targetFrequency=0.0;
    envelope=0.0f;
    targetAmplitude=0.0f;
    bendCents=0.0f;
    currentSourceMidi=-1;
    currentBassMidi=-1;
    gate=false;
    publishedActive.store(false,std::memory_order_relaxed);
    publishedSourceMidi.store(-1,std::memory_order_relaxed);
    publishedBassMidi.store(-1,std::memory_order_relaxed);
    publishedFrequency.store(0.0f,std::memory_order_relaxed);
}

void BassInstrumentEngine::setLevel(float newLevel) noexcept
{
    if(!std::isfinite(newLevel))return;
    outputLevel.store(std::clamp(newLevel,0.0f,1.0f),std::memory_order_relaxed);
}

void BassInstrumentEngine::handlePerformanceEvent(const performance::PerformanceEvent& event) noexcept
{
    if(event.source!=performance::PerformanceEventSource::resolved)return;

    switch(event.type)
    {
        case performance::PerformanceEventType::noteOn:
        {
            if(event.midiNote<0||event.midiNote>127)return;
            currentSourceMidi=event.midiNote;
            currentBassMidi=std::clamp(event.midiNote-12,0,127);
            bendCents=0.0f;
            const float velocity=std::isfinite(event.value)?std::clamp(event.value,0.0f,1.0f):0.0f;
            targetAmplitude=0.16f+0.30f*velocity;
            targetFrequency=midiFrequency(currentBassMidi,bendCents);
            if(frequency<=0.0)frequency=targetFrequency;
            gate=true;
            publishedActive.store(true,std::memory_order_relaxed);
            publishedSourceMidi.store(currentSourceMidi,std::memory_order_relaxed);
            publishedBassMidi.store(currentBassMidi,std::memory_order_relaxed);
            publishedFrequency.store(static_cast<float>(targetFrequency),std::memory_order_relaxed);
            break;
        }
        case performance::PerformanceEventType::noteOff:
            if(event.midiNote==currentSourceMidi)
            {
                gate=false;
                targetAmplitude=0.0f;
                publishedActive.store(false,std::memory_order_relaxed);
            }
            break;
        case performance::PerformanceEventType::pitchBend:
            if(event.midiNote==currentSourceMidi && currentBassMidi>=0)
            {
                bendCents=std::isfinite(event.value)?std::clamp(event.value,-200.0f,200.0f):0.0f;
                targetFrequency=midiFrequency(currentBassMidi,bendCents);
                publishedFrequency.store(static_cast<float>(targetFrequency),std::memory_order_relaxed);
            }
            break;
        case performance::PerformanceEventType::chordChanged:
            break;
    }
}

void BassInstrumentEngine::render(float* output,int numSamples) noexcept
{
    if(output==nullptr||numSamples<=0)return;
    if(!enabled.load(std::memory_order_relaxed))
    {
        std::fill_n(output,numSamples,0.0f);
        return;
    }

    const double safeRate=sampleRate>1000.0?sampleRate:48000.0;
    const float attack=static_cast<float>(1.0-std::exp(-1.0/(0.006*safeRate)));
    const float release=static_cast<float>(1.0-std::exp(-1.0/(0.080*safeRate)));
    const double glide=1.0-std::exp(-1.0/(0.004*safeRate));
    const float levelValue=outputLevel.load(std::memory_order_relaxed);

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

        phase+=twoPi*frequency/safeRate;
        if(phase>=twoPi)phase-=twoPi;
        const double tone=std::sin(phase)+0.24*std::sin(phase*2.0);
        output[i]=static_cast<float>(tone*static_cast<double>(envelope*levelValue));
    }
}
} // namespace pmx::instruments
