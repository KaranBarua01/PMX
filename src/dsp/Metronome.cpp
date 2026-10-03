#include "Metronome.h"
#include <cmath>
namespace pmx::dsp
{
void Metronome::process(float* b,int n) noexcept
{
    if(!enabled||!b||sr<=0)return;
    const double spb=tempo.samplesPerBeat(sr);
    for(int i=0;i<n;++i){ if(phase>=spb){phase-=spb;clickRemaining=static_cast<int>(sr*0.012);} if(clickRemaining>0){ const float env=static_cast<float>(clickRemaining)/(sr*0.012f); b[i]+=level*env*std::sin(2.0*3.14159265*1400.0*(0.012-clickRemaining/sr)); --clickRemaining;} phase+=1.0; }
}
}
