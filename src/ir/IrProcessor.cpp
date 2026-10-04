#include "IrProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace pmx::ir
{
namespace
{
std::uint16_t u16(std::istream& f){ unsigned char b[2]{};f.read(reinterpret_cast<char*>(b),2);return static_cast<std::uint16_t>(b[0]|(b[1]<<8)); }
std::uint32_t u32(std::istream& f){ unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);return static_cast<std::uint32_t>(b[0]|(b[1]<<8)|(b[2]<<16)|(b[3]<<24)); }
bool tag(std::istream&f,const char*expected){char b[4]{};f.read(b,4);return f&&std::memcmp(b,expected,4)==0;}
}
IrLoadResult IrProcessor::loadWav(const std::filesystem::path& path,double targetRate,std::size_t maxFrames)
{
    std::ifstream f(path,std::ios::binary); if(!f)return {false,"IR file could not be opened.",{}};
    if(!tag(f,"RIFF"))return {false,"Not a RIFF WAV file.",{}}; (void)u32(f); if(!tag(f,"WAVE"))return {false,"Not a WAVE file.",{}};
    std::uint16_t format=0,channels=0,bits=0; std::uint32_t sampleRate=0; std::vector<char> raw; bool haveFmt=false;
    while(f && (!haveFmt||raw.empty()))
    {
        char id[4]{}; f.read(id,4); if(!f)break; const auto size=u32(f); const std::string chunk(id,4);
        if(chunk=="fmt ") { format=u16(f);channels=u16(f);sampleRate=u32(f);(void)u32(f);(void)u16(f);bits=u16(f); if(size>16)f.seekg(size-16,std::ios::cur); haveFmt=true; }
        else if(chunk=="data") { raw.resize(size);f.read(raw.data(),static_cast<std::streamsize>(size)); }
        else f.seekg(size,std::ios::cur);
        if(size&1)f.seekg(1,std::ios::cur);
    }
    if(!haveFmt||raw.empty())return {false,"WAV is missing format or audio data.",{}};
    if(channels!=1)return {false,"PMX foundation accepts mono cabinet IRs only.",{}};
    if(sampleRate==0)return {false,"IR sample rate is invalid.",{}};
    std::vector<float> source;
    if(format==1&&bits==16){const auto frames=raw.size()/2;source.resize(frames);for(size_t i=0;i<frames;++i){const auto lo=static_cast<unsigned char>(raw[i*2]);const auto hi=static_cast<unsigned char>(raw[i*2+1]);const auto s=static_cast<std::int16_t>(static_cast<std::uint16_t>(lo|(hi<<8)));source[i]=static_cast<float>(s)/32768.0f;}}
    else if(format==3&&bits==32){const auto frames=raw.size()/4;source.resize(frames);for(size_t i=0;i<frames;++i){float v{};std::memcpy(&v,raw.data()+i*4,4);source[i]=std::isfinite(v)?v:0.0f;}}
    else return {false,"Only mono PCM16 or float32 WAV IRs are supported in this alpha.",{}};
    if(source.empty())return {false,"IR contains no samples.",{}};
    auto out=std::make_shared<IrData>(); out->sampleRate=targetRate;
    if(std::abs(static_cast<double>(sampleRate)-targetRate)<0.5) out->taps=std::move(source);
    else { const double ratio=targetRate/static_cast<double>(sampleRate); const size_t count=static_cast<size_t>(std::ceil(source.size()*ratio)); out->taps.resize(count); for(size_t i=0;i<count;++i){const double pos=i/ratio;const size_t a=std::min(static_cast<size_t>(pos),source.size()-1);const size_t b=std::min(a+1,source.size()-1);const float frac=static_cast<float>(pos-a);out->taps[i]=source[a]+(source[b]-source[a])*frac;} }
    if(out->taps.size()>maxFrames)return {false,"IR is longer than the configured safe limit.",{}};
    return {true,{},std::move(out)};
}
void IrProcessor::setPrepared(std::shared_ptr<const IrData> d){data=std::move(d);history.assign(data?data->taps.size():0,0.0f);write=0;}
void IrProcessor::swapPrepared(IrProcessor& other) noexcept {data.swap(other.data);history.swap(other.history);std::swap(write,other.write);lowState=highState=0;}
void IrProcessor::reset() noexcept {std::fill(history.begin(),history.end(),0.0f);write=0;}
void IrProcessor::process(float* b,int n) noexcept {if(!b||!data||history.empty())return;const auto& taps=data->taps;for(int i=0;i<n;++i){history[write]=b[i];float y=0;size_t idx=write;for(size_t k=0;k<taps.size();++k){y+=history[idx]*taps[k];if(idx==0)idx=history.size()-1;else --idx;}b[i]=y;if(++write>=history.size())write=0;}}
}
