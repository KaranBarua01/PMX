#include <algorithm>
#include <array>
#include <cmath>
#include "dsp/GuitarDrumTrigger.h"

namespace
{
template <std::size_t N>
void makePluck(std::array<float,N>& buffer,double frequency,double sampleRate,double amplitude=0.38)
{
    for(std::size_t i=0;i<N;++i)
    {
        const auto t=static_cast<double>(i)/sampleRate;
        const auto envelope=std::exp(-t*6.0);
        buffer[i]=static_cast<float>(amplitude*envelope*(std::sin(2.0*3.141592653589793*frequency*t)+0.22*std::sin(4.0*3.141592653589793*frequency*t)));
    }
}
}

int main()
{
    pmx::dsp::GuitarDrumTrigger trigger;
    std::array<float,1536> input{}, output{};
    constexpr std::array<double,6> frequencies{82.4069,110.0,146.832,195.998,246.942,329.628};
    for(std::size_t index=0;index<frequencies.size();++index)
    {
        trigger.prepare(44100.0);
        trigger.setEnabled(true);
        input.fill(0.0f); output.fill(0.0f);
        makePluck(input,frequencies[index],44100.0);
        trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
        if(trigger.lastDetectedString()!=static_cast<int>(index)) return static_cast<int>(1+index);
        if(*std::max_element(output.begin(),output.end())<=0.001f) return 10+static_cast<int>(index);
    }

    trigger.prepare(44100.0);
    trigger.setEnabled(true);
    input.fill(0.0f); output.fill(0.0f);
    makePluck(input,frequencies[0],44100.0,0.012);
    trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
    if(trigger.lastDetectedString()!=0) return 22;

    trigger.prepare(44100.0);
    trigger.setEnabled(false);
    input.fill(0.2f); output.fill(0.0f);
    trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
    if(trigger.lastDetectedString()!=-1) return 20;
    for(float v:output) if(std::abs(v)>0.000001f) return 21;

    // A single ringing pluck must produce one drum hit only. The old onset-only
    // cooldown could re-trigger on later waveform peaks while the string rang.
    trigger.prepare(44100.0);
    trigger.setEnabled(true);
    std::array<float,22050> ringing{}, ringingOut{};
    makePluck(ringing,frequencies[0],44100.0,0.38);
    trigger.process(ringing.data(),ringingOut.data(),static_cast<int>(ringing.size()));
    if(trigger.triggerSerial()!=1) return 23;
    if(trigger.lastDetectedString()!=0) return 24;

    // Damping/releasing the string rearms the trigger so the next intentional
    // pluck is still responsive instead of relying on a long fixed cooldown.
    std::array<float,4096> silence{}, silenceOut{}, repluck{}, repluckOut{};
    trigger.process(silence.data(),silenceOut.data(),static_cast<int>(silence.size()));
    makePluck(repluck,frequencies[0],44100.0,0.38);
    trigger.process(repluck.data(),repluckOut.data(),static_cast<int>(repluck.size()));
    if(trigger.triggerSerial()!=2) return 25;
    if(trigger.lastDetectedString()!=0) return 26;

    return 0;
}
