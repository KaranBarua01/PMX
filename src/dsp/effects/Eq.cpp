#include "Eq.h"
#include <cmath>
namespace pmx::dsp { void Eq::process(float* b,int n) noexcept { if(!b)return; const float a=std::exp(-2.0f*3.14159265f*500.0f/static_cast<float>(sampleRate)); const float gl=std::pow(10.0f,lowDb/20.0f), gm=std::pow(10.0f,midDb/20.0f), gh=std::pow(10.0f,highDb/20.0f); for(int i=0;i<n;++i){ const float x=b[i]; lowState=(1-a)*x+a*lowState; const float high=x-lowState; const float mid=x-(lowState+high*0.5f); b[i]=lowState*gl+mid*gm+high*gh; } } }
