#include <array>
#include <cmath>
#include <limits>
#include "analysis/MusicalAnalysisEngine.h"

namespace
{
constexpr double pi = 3.14159265358979323846;

template <std::size_t N>
void fillTone(std::array<float,N>& block,double frequency,double sampleRate,std::uint64_t startSample,float amplitude=0.12f)
{
    for(std::size_t i=0;i<N;++i)
    {
        const double t=static_cast<double>(startSample+i)/sampleRate;
        block[i]=static_cast<float>(amplitude*(std::sin(2.0*pi*frequency*t)+0.18*std::sin(4.0*pi*frequency*t)));
    }
}
}

int main()
{
    using pmx::analysis::MusicalAnalysisEngine;
    using pmx::analysis::PitchMotion;

    constexpr double sampleRate=44100.0;
    MusicalAnalysisEngine engine;
    engine.prepare(sampleRate);

    auto initial=engine.snapshot();
    if(initial.noteActive || initial.frequencyHz!=0.0 || initial.midiNote!=-1) return 1;

    std::array<float,128> block{};
    std::uint64_t samplePosition=0;
    for(int n=0;n<40;++n)
    {
        fillTone(block,110.0,sampleRate,samplePosition);
        engine.process(block.data(),static_cast<int>(block.size()));
        samplePosition+=block.size();
    }

    auto a=engine.snapshot();
    if(!a.noteActive) return 2;
    if(a.onsetSerial!=1) return 3;
    if(a.releaseSerial!=0) return 4;
    if(a.frequencyHz<107.0 || a.frequencyHz>113.0) return 5;
    if(a.midiNote!=45) return 6;
    if(a.confidence<=0.05f) return 7;
    if(a.attack<=0.0f) return 8;

    // A sustained tone must not be mistaken for repeated pick attacks.
    for(int n=0;n<30;++n)
    {
        fillTone(block,110.0,sampleRate,samplePosition);
        engine.process(block.data(),static_cast<int>(block.size()));
        samplePosition+=block.size();
    }
    if(engine.snapshot().onsetSerial!=1) return 9;

    // Moving to a higher pitch must eventually publish positive pitch motion.
    bool sawRising=false;
    for(int n=0;n<48;++n)
    {
        fillTone(block,123.4708,sampleRate,samplePosition);
        engine.process(block.data(),static_cast<int>(block.size()));
        samplePosition+=block.size();
        const auto current=engine.snapshot();
        if(current.motion==PitchMotion::rising && current.pitchDeltaCents>7.0f)
            sawRising=true;
    }
    const auto b=engine.snapshot();
    if(b.frequencyHz<120.0 || b.frequencyHz>127.0) return 10;
    if(b.midiNote!=47) return 11;
    if(!sawRising) return 12;

    // Silence should create a single release and clear the current pitch.
    block.fill(0.0f);
    for(int n=0;n<40;++n)
        engine.process(block.data(),static_cast<int>(block.size()));

    const auto released=engine.snapshot();
    if(released.noteActive) return 13;
    if(released.releaseSerial!=1) return 14;
    if(released.frequencyHz!=0.0 || released.midiNote!=-1) return 15;

    // Non-finite input is treated as silence rather than poisoning analysis state.
    block.fill(0.0f);
    block[10]=std::numeric_limits<float>::quiet_NaN();
    engine.process(block.data(),static_cast<int>(block.size()));
    const auto safe=engine.snapshot();
    if(!std::isfinite(safe.amplitude) || !std::isfinite(safe.confidence)) return 16;

    return 0;
}
