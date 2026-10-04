#include "IrProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#if defined(PMX_HAS_JUCE_DSP)
#include <juce_dsp/juce_dsp.h>
#endif
namespace pmx::ir {
struct IrProcessor::Impl {
#if defined(PMX_HAS_JUCE_DSP)
 juce::dsp::Convolution convolution;
 int maxBlock{};
#endif
};
IrProcessor::IrProcessor()=default;
IrProcessor::~IrProcessor()=default;
namespace {
std::uint16_t u16(std::istream& f){unsigned char b[2]{};f.read(reinterpret_cast<char*>(b),2);return static_cast<std::uint16_t>(b[0]|(b[1]<<8));}
std::uint32_t u32(std::istream& f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
bool tag(std::istream& f,const char* expected){char b[4]{};f.read(b,4);return f&&std::memcmp(b,expected,4)==0;}
}
IrLoadResult IrProcessor::loadWav(const std::filesystem::path& path,double targetRate,std::size_t maxFrames){
 if(!std::isfinite(targetRate)||targetRate<8000||targetRate>192000||maxFrames==0||maxFrames>65536)return {false,"IR preparation settings are invalid.",{}};
 std::error_code ec;auto fileBytes=std::filesystem::file_size(path,ec);
 if(ec||fileBytes<44||fileBytes>16*1024*1024)return {false,"Choose a valid WAV impulse response smaller than 16 MB.",{}};
 std::ifstream f(path,std::ios::binary);if(!f)return {false,"IR file could not be opened.",{}};
 if(!tag(f,"RIFF"))return {false,"Not a RIFF WAV file.",{}};
 const auto riffBytes=u32(f);if(std::uint64_t(riffBytes)+8>fileBytes||riffBytes<36||!tag(f,"WAVE"))return {false,"WAV header is incomplete or invalid.",{}};
 const auto end=std::uint64_t(riffBytes)+8;
 std::uint16_t format{},channels{},bits{},blockAlign{};std::uint32_t sampleRate{};std::vector<char> raw;bool haveFmt=false;
 while(f&&static_cast<std::uint64_t>(f.tellg())+8<=end&&(!haveFmt||raw.empty())){
  char id[4]{};f.read(id,4);auto bytes=u32(f);auto begin=static_cast<std::uint64_t>(f.tellg());
  if(!f||begin+bytes+(bytes&1)>end)return {false,"WAV contains a truncated audio chunk.",{}};
  std::string chunk(id,4);
  if(chunk=="fmt "){
   if(bytes<16)return {false,"WAV format chunk is incomplete.",{}};
   format=u16(f);channels=u16(f);sampleRate=u32(f);(void)u32(f);blockAlign=u16(f);bits=u16(f);
   if(bytes>16)f.seekg(bytes-16,std::ios::cur);haveFmt=true;
  }else if(chunk=="data"){if(bytes==0)return {false,"IR contains no samples.",{}};raw.resize(bytes);f.read(raw.data(),bytes);}
  else f.seekg(bytes,std::ios::cur);
  if(bytes&1)f.seekg(1,std::ios::cur);
 }
 if(!f||!haveFmt||raw.empty())return {false,"WAV is missing complete format or audio data.",{}};
 if(channels!=1)return {false,"Choose a mono cabinet IR. Stereo IRs are not supported in this alpha.",{}};
 if(sampleRate<8000||sampleRate>192000)return {false,"IR sample rate is unsupported.",{}};
 if(!((format==1&&(bits==16||bits==24||bits==32))||(format==3&&bits==32)))return {false,"Use a mono PCM16, PCM24, PCM32 or float32 WAV IR.",{}};
 const auto bytes=static_cast<std::size_t>(bits/8);if(blockAlign!=bytes||raw.size()%bytes!=0)return {false,"WAV sample layout is invalid.",{}};
 auto frames=raw.size()/bytes;double ratio=targetRate/sampleRate;
 auto count=static_cast<std::size_t>(std::ceil(frames*ratio));if(count==0||count>maxFrames)return {false,"IR is too long. Choose an impulse of 8192 samples or fewer at the current rate.",{}};
 std::vector<float> source(frames);double energy{};
 for(std::size_t i=0;i<frames;++i){
  const auto* b=reinterpret_cast<const unsigned char*>(raw.data()+i*bytes);float value{};
  if(format==3)std::memcpy(&value,b,4);
  else if(bits==16)value=static_cast<float>(static_cast<std::int16_t>(std::uint16_t(b[0])|(std::uint16_t(b[1])<<8)))/32768.0f;
  else if(bits==24){std::int32_t v=std::int32_t(b[0])|(std::int32_t(b[1])<<8)|(std::int32_t(b[2])<<16);if(v&0x800000)v-=0x1000000;value=static_cast<float>(v)/8388608.0f;}
  else{std::uint32_t v=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);value=static_cast<float>(static_cast<std::int32_t>(v))/2147483648.0f;}
  if(!std::isfinite(value)||std::abs(value)>32)return {false,"IR contains invalid or excessive sample values.",{}};
  source[i]=value;energy+=std::abs(value);
 }
 if(energy<1e-9)return {false,"IR is silent. Choose a cabinet impulse with audio.",{}};
 auto out=std::make_shared<IrData>();out->sampleRate=targetRate;out->taps.resize(count);
 for(std::size_t i=0;i<count;++i){double pos=i/ratio;auto a=std::min(static_cast<std::size_t>(pos),source.size()-1);auto b=std::min(a+1,source.size()-1);float frac=static_cast<float>(pos-a);out->taps[i]=source[a]+(source[b]-source[a])*frac;}
 return {true,{},std::move(out)};
}
void IrProcessor::setPrepared(std::shared_ptr<const IrData> next,int maxBlock){
 if(next&&(next->taps.empty()||next->sampleRate<=0||maxBlock<=0))throw std::invalid_argument("IR was not prepared");
 auto prepared=std::make_unique<Impl>();
#if defined(PMX_HAS_JUCE_DSP)
 if(next){
  juce::AudioBuffer<float> impulse(1,static_cast<int>(next->taps.size()));std::copy(next->taps.begin(),next->taps.end(),impulse.getWritePointer(0));
  prepared->convolution.loadImpulseResponse(std::move(impulse),next->sampleRate,juce::dsp::Convolution::Stereo::no,juce::dsp::Convolution::Trim::no,juce::dsp::Convolution::Normalise::no);
  prepared->convolution.prepare({next->sampleRate,static_cast<juce::uint32>(maxBlock),1});prepared->maxBlock=maxBlock;
 }
#else
 history.assign(next?next->taps.size():0,0.0f);
#endif
 data=std::move(next);impl=std::move(prepared);write=0;lowState=highState=0;
}
void IrProcessor::swapPrepared(IrProcessor& other) noexcept{data.swap(other.data);history.swap(other.history);impl.swap(other.impl);std::swap(write,other.write);lowState=highState=0;}
void IrProcessor::reset() noexcept{
 std::fill(history.begin(),history.end(),0.0f);write=0;lowState=highState=0;
#if defined(PMX_HAS_JUCE_DSP)
 if(impl)impl->convolution.reset();
#endif
}
void IrProcessor::process(float* b,int n) noexcept{
 if(!b||n<=0||!data)return;
#if defined(PMX_HAS_JUCE_DSP)
 if(!impl||n>impl->maxBlock){std::fill_n(b,n,0.0f);return;}
 float* channels[]{b};juce::dsp::AudioBlock<float> block(channels,1,static_cast<std::size_t>(n));
 juce::dsp::ProcessContextReplacing<float> context(block);impl->convolution.process(context);
#else
 if(history.empty())return;const auto& taps=data->taps;
 for(int i=0;i<n;++i){history[write]=b[i];float y=0;auto idx=write;for(std::size_t k=0;k<taps.size();++k){y+=history[idx]*taps[k];if(idx==0)idx=history.size()-1;else --idx;}b[i]=y;if(++write>=history.size())write=0;}
#endif
 if(filtersEnabled.load()){
  float lo=1-std::exp(-6.2831853f*lowCut.load()/static_cast<float>(data->sampleRate));
  float hi=1-std::exp(-6.2831853f*std::min(highCut.load(),static_cast<float>(data->sampleRate*.45))/static_cast<float>(data->sampleRate));
  float gain=std::pow(10.0f,outputDb.load()/20);
  for(int i=0;i<n;++i){lowState+=lo*(b[i]-lowState);highState+=hi*(b[i]-lowState-highState);b[i]=highState*gain;}
 }
}
}

