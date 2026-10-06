#include <array>
#include <algorithm>
#include <cmath>
#include "instruments/SynthInstrumentEngine.h"

int main()
{
    pmx::instruments::SynthInstrumentEngine synth;
    synth.prepare(48000.0);
    synth.setEnabled(true);
    synth.setLevel(0.7f);
    synth.setBrightness(0.6f);

    pmx::performance::PerformanceEvent on;
    on.type=pmx::performance::PerformanceEventType::noteOn;
    on.source=pmx::performance::PerformanceEventSource::resolved;
    on.midiNote=69;
    on.value=1.0f;
    synth.handlePerformanceEvent(on);

    if(!synth.noteActive())return 1;
    if(synth.midiNote()!=69)return 2;
    if(std::abs(synth.frequencyHz()-440.0f)>0.2f)return 3;

    std::array<float,4096> audio{};
    synth.render(audio.data(),static_cast<int>(audio.size()));
    float peak=0.0f;
    for(float sample:audio)
    {
        if(!std::isfinite(sample))return 4;
        peak=std::max(peak,std::abs(sample));
    }
    if(peak<0.02f||peak>0.8f)return 5;

    pmx::performance::PerformanceEvent bend;
    bend.type=pmx::performance::PerformanceEventType::pitchBend;
    bend.source=pmx::performance::PerformanceEventSource::resolved;
    bend.midiNote=69;
    bend.value=100.0f;
    synth.handlePerformanceEvent(bend);
    if(std::abs(synth.frequencyHz()-466.164f)>0.3f)return 6;

    pmx::performance::PerformanceEvent poly;
    poly.type=pmx::performance::PerformanceEventType::noteOn;
    poly.source=pmx::performance::PerformanceEventSource::polyphonic;
    poly.midiNote=60;
    synth.handlePerformanceEvent(poly);
    if(synth.midiNote()!=69)return 7;

    pmx::performance::PerformanceEvent off;
    off.type=pmx::performance::PerformanceEventType::noteOff;
    off.source=pmx::performance::PerformanceEventSource::resolved;
    off.midiNote=69;
    synth.handlePerformanceEvent(off);
    if(synth.noteActive())return 8;

    std::array<float,16384> release{};
    synth.render(release.data(),static_cast<int>(release.size()));
    float firstPeak=0.0f,lastPeak=0.0f;
    for(std::size_t i=0;i<2048;++i)firstPeak=std::max(firstPeak,std::abs(release[i]));
    for(std::size_t i=release.size()-2048;i<release.size();++i)lastPeak=std::max(lastPeak,std::abs(release[i]));
    if(!(lastPeak<firstPeak*0.45f))return 9;

    synth.setEnabled(false);
    audio.fill(1.0f);
    synth.render(audio.data(),static_cast<int>(audio.size()));
    for(float sample:audio)if(sample!=0.0f)return 10;

    synth.setBrightness(2.0f);
    if(std::abs(synth.brightness()-1.0f)>0.0001f)return 11;
    synth.setBrightness(-1.0f);
    if(std::abs(synth.brightness())>0.0001f)return 12;
    return 0;
}
