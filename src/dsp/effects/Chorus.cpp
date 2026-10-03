#include "Chorus.h"
#include <algorithm>
#include <cmath>
namespace pmx::dsp { void Chorus::prepare(double sr){sampleRate=sr;ring.assign(static_cast<size_t>(sr*0.05)+2u,0.0f);write=0;phase=0;} void Chorus::process(float* b,int n) noexcept { if(!b||ring.empty())return; for(int i=0;i<n;++i){ const float mod=(std::sin(phase)+1.0f)*0.5f; const int d=std::clamp(static_cast<int>(sampleRate*(0.008f+0.012f*depth*mod)),1,static_cast<int>(ring.size())-1); int read=write-d;if(read<0)read+=static_cast<int>(ring.size()); const float wet=ring[static_cast<size_t>(read)],x=b[i]; ring[static_cast<size_t>(write)]=x; b[i]=x*(1-mix)+wet*mix; if(++write>=static_cast<int>(ring.size()))write=0; phase+=2.0f*3.14159265f*rateHz/static_cast<float>(sampleRate); if(phase>2.0f*3.14159265f)phase-=2.0f*3.14159265f;} } }
