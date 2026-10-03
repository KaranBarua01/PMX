#include "Delay.h"
#include <algorithm>
#include <cmath>
namespace pmx::dsp { void Delay::prepare(double sr,int,float maxDelayMs){ sampleRate=sr; ring.assign(static_cast<size_t>(std::ceil(sr*maxDelayMs/1000.0))+2u,0.0f); write=0; } void Delay::process(float* b,int n) noexcept { if(!b||ring.empty())return; int d=static_cast<int>(std::round(sampleRate*timeMs/1000.0)); d=std::clamp(d,1,static_cast<int>(ring.size())-1); for(int i=0;i<n;++i){ int read=write-d; if(read<0)read+=static_cast<int>(ring.size()); const float delayed=ring[static_cast<size_t>(read)]; const float input=b[i]; ring[static_cast<size_t>(write)]=input+delayed*feedback; b[i]=input*(1.0f-mix)+delayed*mix; if(++write>=static_cast<int>(ring.size()))write=0; } } }
