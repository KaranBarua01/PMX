#include "Delay.h"
#include <algorithm>
#include <cmath>
namespace pmx::dsp {
void Delay::prepare(double sr,int,float maxDelayMs){sampleRate=sr;ring.assign(static_cast<std::size_t>(std::ceil(sr*maxDelayMs/1000))+2,0);write=0;initialised=false;}
void Delay::process(float* b,int n) noexcept{
 if(!b||ring.empty())return;
 const float targetDelay=std::clamp(static_cast<float>(sampleRate*timeMs.load()/1000),1.0f,static_cast<float>(ring.size()-1));
 const float targetFeedback=feedback.load(),targetMix=mix.load();
 if(!initialised){currentDelay=targetDelay;currentFeedback=targetFeedback;currentMix=targetMix;initialised=true;}
 const float smooth=1-std::exp(-1.0f/static_cast<float>(sampleRate*.02));
 for(int i=0;i<n;++i){
  currentDelay+=smooth*(targetDelay-currentDelay);currentFeedback+=smooth*(targetFeedback-currentFeedback);currentMix+=smooth*(targetMix-currentMix);
  float pos=static_cast<float>(write)-currentDelay;if(pos<0)pos+=static_cast<float>(ring.size());
  auto a=static_cast<std::size_t>(pos);auto next=(a+1)%ring.size();float fraction=pos-static_cast<float>(a);
  float wet=ring[a]+fraction*(ring[next]-ring[a]);float x=b[i];
  ring[static_cast<std::size_t>(write)]=x+wet*currentFeedback;b[i]=x*(1-currentMix)+wet*currentMix;
  if(++write>=static_cast<int>(ring.size()))write=0;
 }
}
}
