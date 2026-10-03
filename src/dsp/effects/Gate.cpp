#include "Gate.h"
#include <cmath>
namespace pmx::dsp { void Gate::process(float* b,int n) noexcept { if(!b) return; const float t=std::pow(10.0f,thresholdDb/20.0f); for(int i=0;i<n;++i) if(std::abs(b[i])<t) b[i]=0.0f; } }
