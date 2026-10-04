#include "Gate.h"
#include <cmath>
namespace pmx::dsp
{
void Gate::process(float* b,int n) noexcept
{
    if(!b) return;
    const float threshold=std::pow(10.0f,thresholdDb.load()/20.0f);
    const float attack=std::exp(-1.0f/static_cast<float>(sampleRate*attackMs.load()/1000));
    const float release=std::exp(-1.0f/static_cast<float>(sampleRate*releaseMs.load()/1000));
    for(int i=0;i<n;++i)
    {
        const float x=std::isfinite(b[i])?b[i]:0;
        const float magnitude=std::abs(x);
        envelope=std::max(magnitude, envelope*release);
        const float target=envelope>threshold?1.0f:0.0f;
        const float coefficient=target>gain?attack:release;
        gain=target+(gain-target)*coefficient;
        b[i]=x*gain;
    }
}
}
