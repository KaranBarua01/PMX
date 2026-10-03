#include "Compressor.h"
#include <cmath>
namespace pmx::dsp { void Compressor::process(float* b,int n) noexcept { if(!b)return; const float t=std::pow(10.0f,thresholdDb/20.0f); for(int i=0;i<n;++i){ const float s=b[i], a=std::abs(s); if(a>t){ const float compressed=t+(a-t)/ratio; b[i]=std::copysign(compressed,s); } } } }
