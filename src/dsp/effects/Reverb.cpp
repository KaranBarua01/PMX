#include "Reverb.h"
namespace pmx::dsp
{
void Reverb::prepare(double sr)
{
    const double seconds[]{0.0311,0.0371,0.0411,0.0437};
    for(std::size_t c=0;c<4;++c) rings[c].assign(static_cast<std::size_t>(sr*seconds[c])+1,0);
    write.fill(0); filtered.fill(0);
}
void Reverb::process(float* b,int n) noexcept
{
    if(!b||rings[0].empty()) return;
    const float amount=mix.load(), feedback=decay.load(), damping=0.05f+0.9f*tone.load();
    for(int i=0;i<n;++i)
    {
        const float x=b[i]; float wet=0;
        for(std::size_t c=0;c<4;++c)
        {
            auto& ring=rings[c]; const float delayed=ring[write[c]];
            filtered[c]+=damping*(delayed-filtered[c]);
            ring[write[c]]=x*0.25f+filtered[c]*feedback; wet+=delayed;
            if(++write[c]>=ring.size()) write[c]=0;
        }
        b[i]=x*(1-amount)+wet*amount;
    }
}
}
