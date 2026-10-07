#include "instruments/PianoInstrumentEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::instruments
{
namespace
{
constexpr double twoPi=6.283185307179586476925286766559;
}

double PianoInstrumentEngine::midiFrequency(int midi) noexcept
{
    return 440.0*std::pow(2.0,(static_cast<double>(midi)-69.0)/12.0);
}

void PianoInstrumentEngine::prepare(double newSampleRate) noexcept
{
    sampleRate=std::isfinite(newSampleRate) && newSampleRate>1000.0?newSampleRate:48000.0;
    reset();
}

void PianoInstrumentEngine::reset() noexcept
{
    for(auto& voice:voices)voice=Voice{};
    publishedHeldCount.store(0,std::memory_order_relaxed);
}

void PianoInstrumentEngine::setLevel(float newLevel) noexcept
{
    if(!std::isfinite(newLevel))return;
    outputLevel.store(std::clamp(newLevel,0.0f,1.0f),std::memory_order_relaxed);
}

void PianoInstrumentEngine::setBrightness(float newBrightness) noexcept
{
    if(!std::isfinite(newBrightness))return;
    toneBrightness.store(std::clamp(newBrightness,0.0f,1.0f),std::memory_order_relaxed);
}

PianoInstrumentEngine::Voice* PianoInstrumentEngine::findVoice(int midi) noexcept
{
    for(auto& voice:voices)
        if(voice.active && voice.midi==midi)return &voice;
    return nullptr;
}

PianoInstrumentEngine::Voice* PianoInstrumentEngine::allocateVoice(int midi) noexcept
{
    if(auto* existing=findVoice(midi))return existing;

    for(auto& voice:voices)
        if(!voice.active)return &voice;

    auto* quietest=&voices[0];
    for(auto& voice:voices)
        if(voice.envelope<quietest->envelope)quietest=&voice;
    return quietest;
}

void PianoInstrumentEngine::publishHeldCount() noexcept
{
    int count=0;
    for(const auto& voice:voices)if(voice.active && voice.keyDown)++count;
    publishedHeldCount.store(count,std::memory_order_relaxed);
}

void PianoInstrumentEngine::handlePerformanceEvent(const performance::PerformanceEvent& event) noexcept
{
    if(event.source!=performance::PerformanceEventSource::polyphonic)return;

    switch(event.type)
    {
        case performance::PerformanceEventType::noteOn:
        {
            if(event.midiNote<0||event.midiNote>127)return;
            auto* voice=allocateVoice(event.midiNote);
            const float velocity=std::isfinite(event.value)?std::clamp(event.value,0.0f,1.0f):0.0f;
            voice->midi=event.midiNote;
            voice->phase=0.0;
            voice->frequency=midiFrequency(event.midiNote);
            voice->envelope=0.0f;
            voice->strike=0.10f+0.20f*velocity;
            voice->ageSamples=0;
            voice->active=true;
            voice->keyDown=true;
            publishHeldCount();
            break;
        }
        case performance::PerformanceEventType::noteOff:
            if(auto* voice=findVoice(event.midiNote))
            {
                voice->keyDown=false;
                publishHeldCount();
            }
            break;
        case performance::PerformanceEventType::pitchBend:
        case performance::PerformanceEventType::chordChanged:
            break;
    }
}

void PianoInstrumentEngine::render(float* output,int numSamples) noexcept
{
    if(output==nullptr||numSamples<=0)return;
    std::fill_n(output,numSamples,0.0f);
    if(!enabled.load(std::memory_order_relaxed))return;

    const double safeRate=sampleRate>1000.0?sampleRate:48000.0;
    const float attack=static_cast<float>(1.0-std::exp(-1.0/(0.003*safeRate)));
    const float holdDecay=static_cast<float>(std::exp(-1.0/(2.2*safeRate)));
    const float releaseDecay=static_cast<float>(std::exp(-1.0/(0.16*safeRate)));
    const float levelValue=outputLevel.load(std::memory_order_relaxed);
    const float bright=toneBrightness.load(std::memory_order_relaxed);

    for(int i=0;i<numSamples;++i)
    {
        double mixed=0.0;
        for(auto& voice:voices)
        {
            if(!voice.active)continue;

            if(voice.ageSamples<static_cast<std::uint64_t>(safeRate*0.012))
                voice.envelope+=(voice.strike-voice.envelope)*attack;
            else if(voice.keyDown)
            {
                voice.envelope*=holdDecay;
                const float sustain=voice.strike*0.08f;
                if(voice.envelope<sustain)voice.envelope=sustain;
            }
            else
                voice.envelope*=releaseDecay;

            if(!voice.keyDown && voice.envelope<0.00001f)
            {
                voice=Voice{};
                continue;
            }

            voice.phase+=voice.frequency/safeRate;
            voice.phase-=std::floor(voice.phase);
            const double p=twoPi*voice.phase;
            const double fundamental=std::sin(p);
            const double harmonics=0.34*std::sin(2.0*p)+0.16*std::sin(3.0*p)+0.08*std::sin(4.0*p);
            const double tone=fundamental+static_cast<double>(bright)*harmonics;
            mixed+=tone*static_cast<double>(voice.envelope);
            ++voice.ageSamples;
        }
        output[i]=static_cast<float>(mixed*static_cast<double>(levelValue));
    }
}
} // namespace pmx::instruments
