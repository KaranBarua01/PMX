#include <cstdint>
#include <filesystem>
#include <fstream>
#include "ir/IrProcessor.h"

static void put16(std::ofstream& f, std::uint16_t v){ f.put(static_cast<char>(v&0xff)); f.put(static_cast<char>((v>>8)&0xff)); }
static void put32(std::ofstream& f, std::uint32_t v){ for(int i=0;i<4;++i) f.put(static_cast<char>((v>>(8*i))&0xff)); }
static void writeWav(const std::filesystem::path& p, int channels)
{
    const std::int16_t samples[] {32767, 0, -16384, 8192};
    const std::uint32_t dataBytes = sizeof(samples) * static_cast<std::uint32_t>(channels);
    std::ofstream f(p, std::ios::binary);
    f.write("RIFF",4); put32(f,36+dataBytes); f.write("WAVE",4);
    f.write("fmt ",4); put32(f,16); put16(f,1); put16(f,static_cast<std::uint16_t>(channels)); put32(f,48000);
    put32(f,48000*channels*2); put16(f,static_cast<std::uint16_t>(channels*2)); put16(f,16);
    f.write("data",4); put32(f,dataBytes);
    for(auto s:samples) for(int c=0;c<channels;++c) put16(f,static_cast<std::uint16_t>(s));
}
int main()
{
    namespace fs=std::filesystem;
    auto dir=fs::temp_directory_path()/"pmx_ir_test"; fs::create_directories(dir);
    auto mono=dir/"cab.wav"; writeWav(mono,1);
    auto result=pmx::ir::IrProcessor::loadWav(mono,48000.0,8192);
    if(!result.ok||!result.data) return 1;
    if(result.data->taps.size()!=4) return 2;
    if(result.data->sampleRate!=48000.0) return 3;
    if(result.data->taps[0] < 0.99f) return 4;
    auto stereo=dir/"stereo.wav"; writeWav(stereo,2);
    if(pmx::ir::IrProcessor::loadWav(stereo,48000.0,8192).ok) return 5;
    if(pmx::ir::IrProcessor::loadWav(dir/"missing.wav",48000.0,8192).ok) return 6;
    fs::remove_all(dir); return 0;
}
