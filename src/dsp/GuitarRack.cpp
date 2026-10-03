#include "GuitarRack.h"
namespace pmx::dsp
{
void GuitarRack::prepare(double sr, int maxBlock)
{
    eq.prepare(sr); chorus.prepare(sr); delay.prepare(sr,maxBlock,2000.0f); reverb.prepare(sr);
}
void GuitarRack::processPreModels(float* b,int n) noexcept
{
    if(!b||n<=0)return;
    if(isEnabled(RackModule::gate)) gate.process(b,n);
    if(isEnabled(RackModule::comp)) comp.process(b,n);
    if(isEnabled(RackModule::drive)) drive.process(b,n);
}
void GuitarRack::processPostModels(float* b,int n) noexcept
{
    if(!b||n<=0)return;
    if(isEnabled(RackModule::eq)) eq.process(b,n);
    if(isEnabled(RackModule::mod)) chorus.process(b,n);
    if(isEnabled(RackModule::delay)) delay.process(b,n);
    if(isEnabled(RackModule::reverb)) reverb.process(b,n);
}
void GuitarRack::process(float* b,int n) noexcept
{
    processPreModels(b,n);
    processPostModels(b,n);
}
} // namespace pmx::dsp
