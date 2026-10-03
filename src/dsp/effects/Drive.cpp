#include "Drive.h"
#include <cmath>
namespace pmx::dsp { void Drive::process(float* b,int n) noexcept { if(!b)return; for(int i=0;i<n;++i){ current += (target-current)*0.02f; const float gain=1.0f+current*18.0f; const float norm=std::tanh(gain); b[i]=norm>0.0f?std::tanh(b[i]*gain)/norm:b[i]; } } }
