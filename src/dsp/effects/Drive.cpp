#include "Drive.h"
#include <cmath>
namespace pmx::dsp { void Drive::process(float* b,int n) noexcept { if(!b)return; const float amount=target.load(),toneValue=tone.load(),output=std::pow(10.0f,outputDb.load()/20); const float coefficient=std::exp(-2*3.14159265f*(1200+toneValue*10000)/static_cast<float>(sampleRate)); for(int i=0;i<n;++i){ current += (amount-current)*0.02f; const float x=b[i], gain=1+current*18; const float wet=std::tanh(x*gain)/std::tanh(gain); lowState=(1-coefficient)*wet+coefficient*lowState; b[i]=(x*(1-current)+lowState*current)*output; } } }
