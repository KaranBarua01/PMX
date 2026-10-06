#include <array>
#include <algorithm>
#include <cmath>
#include "instruments/BassInstrumentEngine.h"

int main()
{
    pmx::instruments::BassInstrumentEngine bass;
    bass.prepare(48000.0);
    bass.setEnabled(true);
    bass.setLevel(0.7f);

    pmx::performance::PerformanceEvent on;
    on.type=pmx::performance::PerformanceEventType::noteOn;
    on.source=pmx::performance::PerformanceEventSource::resolved;
    on.midiNote=45;
    on.value=1.0f;
    bass.handlePerformanceEvent(on);

    if(!bass.noteActive())return 1;
    if(bass.sourceMidiNote()!=45)return 2;
    if(bass.bassMidiNote()!=33)return 3;
    if(std::abs(bass.frequencyHz()-55.0f)>0.2f)return 4;

    std::array<float,2048> audio{};
    bass.render(audio.data(),static_cast<int>(audio.size()));
    float peak=0.0f;
    for(float sample:audio)
    {
        if(!std::isfinite(sample))return 5;
        peak=std::max(peak,std::abs(sample));
    }
    if(peak<0.02f||peak>0.8f)return 6;

    pmx::performance::PerformanceEvent bend;
    bend.type=pmx::performance::PerformanceEventType::pitchBend;
    bend.source=pmx::performance::PerformanceEventSource::resolved;
    bend.midiNote=45;
    bend.value=100.0f;
    bass.handlePerformanceEvent(bend);
    if(std::abs(bass.frequencyHz()-58.2705f)>0.25f)return 7;

    pmx::performance::PerformanceEvent poly;
    poly.type=pmx::performance::PerformanceEventType::noteOn;
    poly.source=pmx::performance::PerformanceEventSource::polyphonic;
    poly.midiNote=60;
    bass.handlePerformanceEvent(poly);
    if(bass.sourceMidiNote()!=45||bass.bassMidiNote()!=33)return 8;

    pmx::performance::PerformanceEvent off;
    off.type=pmx::performance::PerformanceEventType::noteOff;
    off.source=pmx::performance::PerformanceEventSource::resolved;
    off.midiNote=45;
    bass.handlePerformanceEvent(off);
    if(bass.noteActive())return 9;

    std::array<float,8192> release{};
    bass.render(release.data(),static_cast<int>(release.size()));
    float firstPeak=0.0f,lastPeak=0.0f;
    for(std::size_t i=0;i<1024;++i)firstPeak=std::max(firstPeak,std::abs(release[i]));
    for(std::size_t i=release.size()-1024;i<release.size();++i)lastPeak=std::max(lastPeak,std::abs(release[i]));
    if(!(lastPeak<firstPeak*0.45f))return 10;

    bass.setEnabled(false);
    audio.fill(1.0f);
    bass.render(audio.data(),static_cast<int>(audio.size()));
    for(float sample:audio)if(sample!=0.0f)return 11;

    bass.setLevel(2.0f);
    if(std::abs(bass.level()-1.0f)>0.0001f)return 12;
    bass.setLevel(-1.0f);
    if(std::abs(bass.level())>0.0001f)return 13;
    return 0;
}
