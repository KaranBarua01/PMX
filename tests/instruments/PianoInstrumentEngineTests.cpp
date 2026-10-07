#include <array>
#include <algorithm>
#include <cmath>
#include "instruments/PianoInstrumentEngine.h"

namespace
{
pmx::performance::PerformanceEvent polyEvent(pmx::performance::PerformanceEventType type,int midi,float value=1.0f)
{
    pmx::performance::PerformanceEvent event;
    event.type=type;
    event.source=pmx::performance::PerformanceEventSource::polyphonic;
    event.midiNote=midi;
    event.value=value;
    return event;
}
}

int main()
{
    pmx::instruments::PianoInstrumentEngine piano;
    piano.prepare(48000.0);
    piano.setEnabled(true);
    piano.setLevel(0.7f);
    piano.setBrightness(0.6f);

    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOn,60,0.9f));
    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOn,64,0.8f));
    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOn,67,0.85f));
    if(piano.heldNoteCount()!=3)return 1;

    std::array<float,4096> audio{};
    piano.render(audio.data(),static_cast<int>(audio.size()));
    float peak=0.0f;
    for(float sample:audio)
    {
        if(!std::isfinite(sample))return 2;
        peak=std::max(peak,std::abs(sample));
    }
    if(peak<0.02f||peak>1.5f)return 3;

    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOff,64,0.0f));
    if(piano.heldNoteCount()!=2)return 4;

    std::array<float,32768> release{};
    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOff,60,0.0f));
    piano.handlePerformanceEvent(polyEvent(pmx::performance::PerformanceEventType::noteOff,67,0.0f));
    if(piano.heldNoteCount()!=0)return 5;
    piano.render(release.data(),static_cast<int>(release.size()));
    float tailPeak=0.0f;
    for(std::size_t i=release.size()-2048;i<release.size();++i)tailPeak=std::max(tailPeak,std::abs(release[i]));
    if(tailPeak>0.02f)return 6;

    pmx::performance::PerformanceEvent resolved;
    resolved.type=pmx::performance::PerformanceEventType::noteOn;
    resolved.source=pmx::performance::PerformanceEventSource::resolved;
    resolved.midiNote=69;
    piano.handlePerformanceEvent(resolved);
    if(piano.heldNoteCount()!=0)return 7;

    piano.setEnabled(false);
    audio.fill(1.0f);
    piano.render(audio.data(),static_cast<int>(audio.size()));
    for(float sample:audio)if(sample!=0.0f)return 8;

    piano.setBrightness(2.0f);
    if(std::abs(piano.brightness()-1.0f)>0.0001f)return 9;
    piano.setBrightness(-1.0f);
    if(std::abs(piano.brightness())>0.0001f)return 10;

    piano.setLevel(2.0f);
    if(std::abs(piano.level()-1.0f)>0.0001f)return 11;
    piano.setLevel(-1.0f);
    if(std::abs(piano.level())>0.0001f)return 12;

    piano.reset();
    if(piano.heldNoteCount()!=0)return 13;
    return 0;
}
