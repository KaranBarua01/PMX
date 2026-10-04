#include "Compressor.h"
#include <cmath>
namespace pmx::dsp
{
void Compressor::process(float* b,int n) noexcept
{
    if(!b) return;
    const float threshold=thresholdDb.load(), compressionRatio=ratio.load(), makeup=std::pow(10.0f,makeupDb.load()/20);
    const float attack=std::exp(-1.0f/static_cast<float>(sampleRate*0.005)), release=std::exp(-1.0f/static_cast<float>(sampleRate*0.1));
    for(int i=0;i<n;++i)
    {
        const float x=std::isfinite(b[i])?b[i]:0;
        const float db=20*std::log10(std::max(1e-9f,std::abs(x)));
        const float reduction=std::max(0.0f,db-threshold)*(1-1/compressionRatio);
        const float target=std::pow(10.0f,-reduction/20);
        const float coefficient=target<gain?attack:release;
        gain=target+(gain-target)*coefficient;
        b[i]=x*gain*makeup;
    }
}
}
