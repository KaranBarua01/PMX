#include <array>
#include <algorithm>
#include <cmath>
#include "instruments/ViolinInstrumentEngine.h"

int main()
{
    pmx::instruments::ViolinInstrumentEngine violin;
    violin.prepare(48000.0);
    violin.setEnabled(true);
    violin.setLevel(0.7f);
    violin.setBrightness(0.6f);

    pmx::performance::PerformanceEvent on;
    on.type=pmx::performance::PerformanceEventType::noteOn;
    on.source=pmx::performance::PerformanceEventSource::resolved;
    on.midiNote=69;
    on.value=1.0f;
    violin.handlePerformanceEvent(on);

    if(!violin.noteActive())return 1;
    if(violin.midiNote()!=69)return 2;
    if(std::abs(violin.frequencyHz()-440.0f)>0.2f)return 3;

    std::array<float,8192> audio{};
    violin.render(audio.data(),static_cast<int>(audio.size()));
    float peak=0.0f;
    for(float sample:audio)
    {
        if(!std::isfinite(sample))return 4;
        peak=std::max(peak,std::abs(sample));
    }
    if(peak<0.02f||peak>1.0f)return 5;

    pmx::performance::PerformanceEvent bend;
    bend.type=pmx::performance::PerformanceEventType::pitchBend;
    bend.source=pmx::performance::PerformanceEventSource::resolved;
    bend.midiNote=69;
    bend.value=100.0f;
    violin.handlePerformanceEvent(bend);
    if(std::abs(violin.frequencyHz()-466.164f)>0.3f)return 6;

    pmx::performance::PerformanceEvent poly;
    poly.type=pmx::performance::PerformanceEventType::noteOn;
    poly.source=pmx::performance::PerformanceEventSource::polyphonic;
    poly.midiNote=60;
    violin.handlePerformanceEvent(poly);
    if(violin.midiNote()!=69)return 7;

    pmx::performance::PerformanceEvent off;
    off.type=pmx::performance::PerformanceEventType::noteOff;
    off.source=pmx::performance::PerformanceEventSource::resolved;
    off.midiNote=69;
    violin.handlePerformanceEvent(off);
    if(violin.noteActive())return 8;

    std::array<float,32768> release{};
    violin.render(release.data(),static_cast<int>(release.size()));
    float firstPeak=0.0f,lastPeak=0.0f;
    for(std::size_t i=0;i<2048;++i)firstPeak=std::max(firstPeak,std::abs(release[i]));
    for(std::size_t i=release.size()-2048;i<release.size();++i)lastPeak=std::max(lastPeak,std::abs(release[i]));
    if(!(lastPeak<firstPeak*0.35f))return 9;

    violin.setEnabled(false);
    audio.fill(1.0f);
    violin.render(audio.data(),static_cast<int>(audio.size()));
    for(float sample:audio)if(sample!=0.0f)return 10;

    violin.setBrightness(2.0f);
    if(std::abs(violin.brightness()-1.0f)>0.0001f)return 11;
    violin.setBrightness(-1.0f);
    if(std::abs(violin.brightness())>0.0001f)return 12;

    violin.setLevel(2.0f);
    if(std::abs(violin.level()-1.0f)>0.0001f)return 13;
    violin.setLevel(-1.0f);
    if(std::abs(violin.level())>0.0001f)return 14;

    violin.reset();
    if(violin.noteActive())return 15;
    if(violin.midiNote()!=-1)return 16;
    return 0;
}
