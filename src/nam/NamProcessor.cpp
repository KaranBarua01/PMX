#include "NamProcessor.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>
#if defined(PMX_HAS_NAM_CORE)
#include "NAM/get_dsp.h"
#endif
namespace pmx::nam
{
struct NamProcessor::Impl
{
#if defined(PMX_HAS_NAM_CORE)
    std::unique_ptr<::nam::DSP> model;
    std::vector<float> scratch;
#endif
    float inputGain{1.0f},outputGain{1.0f}; bool isLoaded{false};
};
NamProcessor::NamProcessor():impl(std::make_unique<Impl>()){} NamProcessor::~NamProcessor()=default; NamProcessor::NamProcessor(NamProcessor&&) noexcept=default; NamProcessor& NamProcessor::operator=(NamProcessor&&) noexcept=default;
bool NamProcessor::engineCompiled() noexcept {
#if defined(PMX_HAS_NAM_CORE)
return true;
#else
return false;
#endif
}
NamLoadResult NamProcessor::load(const std::filesystem::path& p,double sr,int maxBlock)
{
    if(!std::filesystem::exists(p)||!std::filesystem::is_regular_file(p))return {false,"NAM file does not exist."};
    auto ext=p.extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});if(ext!=".nam")return {false,"Choose a .nam model file."};
#if defined(PMX_HAS_NAM_CORE)
    try{auto next=::nam::get_dsp(p,{.prewarm=false});if(!next)return {false,"NAM model could not be loaded."};if(next->NumInputChannels()!=1||next->NumOutputChannels()!=1)return {false,"This alpha supports mono-in/mono-out NAM models only."};next->SetPrewarmOnReset(false);next->Reset(sr,maxBlock);next->prewarm();impl->scratch.assign(static_cast<size_t>(maxBlock),0.0f);NamLoadResult result{true,{},next->GetExpectedSampleRate(),next->NumInputChannels(),next->NumOutputChannels()};impl->model=std::move(next);impl->isLoaded=true;return result;}catch(const std::exception&e){return {false,e.what()};}
#else
    (void)sr;(void)maxBlock; return {false,"NAM engine is not compiled in this build."};
#endif
}
void NamProcessor::unload() noexcept {
#if defined(PMX_HAS_NAM_CORE)
impl->model.reset();impl->scratch.clear();
#endif
impl->isLoaded=false;}
void NamProcessor::setInputTrimDb(float db) noexcept{impl->inputGain=std::pow(10.0f,db/20.0f);}void NamProcessor::setOutputTrimDb(float db) noexcept{impl->outputGain=std::pow(10.0f,db/20.0f);}bool NamProcessor::loaded()const noexcept{return impl->isLoaded;}
void NamProcessor::process(float*b,int n) noexcept{if(!b||n<=0||!impl->isLoaded)return;
#if defined(PMX_HAS_NAM_CORE)
if(!impl->model||static_cast<size_t>(n)>impl->scratch.size())return;for(int i=0;i<n;++i)b[i]*=impl->inputGain;NAM_SAMPLE* in[]{b};NAM_SAMPLE* out[]{impl->scratch.data()};impl->model->process(in,out,n);for(int i=0;i<n;++i)b[i]=impl->scratch[static_cast<size_t>(i)]*impl->outputGain;
#endif
}
}
