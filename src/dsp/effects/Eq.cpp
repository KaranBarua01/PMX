#include "Eq.h"
#include <cmath>
namespace pmx::dsp
{
void Eq::process(float* b, int n) noexcept
{
    if (!b) return;
    const float a=std::exp(-2.0f*3.14159265f*250.0f/static_cast<float>(sampleRate));
    const float h=std::exp(-2.0f*3.14159265f*2500.0f/static_cast<float>(sampleRate));
    const float gl=std::pow(10.0f,lowDb.load()/20.0f), gm=std::pow(10.0f,midDb.load()/20.0f), gh=std::pow(10.0f,highDb.load()/20.0f);
    for (int i=0; i<n; ++i)
    {
        const float x=b[i];
        lowState=(1-a)*x+a*lowState;
        highState=(1-h)*x+h*highState;
        b[i]=lowState*gl+(highState-lowState)*gm+(x-highState)*gh;
    }
}
}
