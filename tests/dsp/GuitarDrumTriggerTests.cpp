#include <algorithm>
#include <array>
#include <cmath>
#include "dsp/GuitarDrumTrigger.h"

namespace
{
template <std::size_t N>
void makePluck(std::array<float,N>& buffer,double frequency,double sampleRate)
{
    for(std::size_t i=0;i<N;++i)
    {
        const auto t=static_cast<double>(i)/sampleRate;
        const auto envelope=std::exp(-t*6.0);
        buffer[i]=static_cast<float>(0.38*envelope*(std::sin(2.0*3.141592653589793*frequency*t)+0.22*std::sin(4.0*3.141592653589793*frequency*t)));
    }
}
}

int main()
{
    pmx::dsp::GuitarDrumTrigger trigger;
    trigger.prepare(44100.0);
    trigger.setEnabled(true);

    std::array<float,1536> input{}, output{};
    makePluck(input,82.4069,44100.0);
    trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
    if(trigger.lastDetectedString()!=static_cast<int>(pmx::dsp::GuitarDrumPad::lowE)) return 1;
    if(*std::max_element(output.begin(),output.end())<=0.001f) return 2;

    trigger.prepare(44100.0);
    trigger.setEnabled(true);
    input.fill(0.0f); output.fill(0.0f);
    makePluck(input,110.0,44100.0);
    trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
    if(trigger.lastDetectedString()!=static_cast<int>(pmx::dsp::GuitarDrumPad::a)) return 3;

    trigger.prepare(44100.0);
    trigger.setEnabled(false);
    input.fill(0.2f); output.fill(0.0f);
    trigger.process(input.data(),output.data(),static_cast<int>(input.size()));
    if(trigger.lastDetectedString()!=-1) return 4;
    for(float v:output) if(std::abs(v)>0.000001f) return 5;

    return 0;
}
