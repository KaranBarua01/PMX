#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "analysis/PolyphonicNoteDetector.h"
#include "instruments/GuitarProfile.h"

namespace
{
struct WavData
{
    int sampleRate {};
    int channels {};
    int bitsPerSample {};
    int format {};
    std::vector<float> mono;
};

std::uint16_t readU16(std::istream& in)
{
    unsigned char b[2]{};
    in.read(reinterpret_cast<char*>(b),2);
    return static_cast<std::uint16_t>(b[0] | (static_cast<std::uint16_t>(b[1])<<8));
}

std::uint32_t readU32(std::istream& in)
{
    unsigned char b[4]{};
    in.read(reinterpret_cast<char*>(b),4);
    return static_cast<std::uint32_t>(b[0]) |
           (static_cast<std::uint32_t>(b[1])<<8) |
           (static_cast<std::uint32_t>(b[2])<<16) |
           (static_cast<std::uint32_t>(b[3])<<24);
}

bool readWav(const std::string& path,WavData& wav)
{
    std::ifstream in(path,std::ios::binary);
    if(!in)return false;

    char riff[4]{},wave[4]{};
    in.read(riff,4);
    (void)readU32(in);
    in.read(wave,4);
    if(std::strncmp(riff,"RIFF",4)!=0 || std::strncmp(wave,"WAVE",4)!=0)return false;

    std::vector<unsigned char> data;
    int blockAlign=0;

    while(in)
    {
        char id[4]{};
        if(!in.read(id,4))break;
        const auto size=readU32(in);
        const auto next=static_cast<std::streamoff>(size + (size&1u));

        if(std::strncmp(id,"fmt ",4)==0)
        {
            wav.format=readU16(in);
            wav.channels=readU16(in);
            wav.sampleRate=static_cast<int>(readU32(in));
            (void)readU32(in);
            blockAlign=readU16(in);
            wav.bitsPerSample=readU16(in);
            if(size>16)in.seekg(static_cast<std::streamoff>(size-16),std::ios::cur);
            if(size&1u)in.seekg(1,std::ios::cur);
        }
        else if(std::strncmp(id,"data",4)==0)
        {
            data.resize(size);
            if(size>0)in.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(size));
            if(size&1u)in.seekg(1,std::ios::cur);
        }
        else
        {
            in.seekg(next,std::ios::cur);
        }
    }

    if(wav.sampleRate<=0 || wav.channels<=0 || blockAlign<=0 || data.empty())return false;
    if(wav.format!=1 && wav.format!=3)return false;

    const std::size_t frames=data.size()/static_cast<std::size_t>(blockAlign);
    wav.mono.resize(frames);

    for(std::size_t frame=0;frame<frames;++frame)
    {
        double sum=0.0;
        for(int ch=0;ch<wav.channels;++ch)
        {
            const auto* p=data.data()+frame*static_cast<std::size_t>(blockAlign)
                         + static_cast<std::size_t>(ch*wav.bitsPerSample/8);
            float value=0.0f;

            if(wav.format==3 && wav.bitsPerSample==32)
            {
                std::memcpy(&value,p,4);
            }
            else if(wav.format==1 && wav.bitsPerSample==16)
            {
                const auto s=static_cast<std::int16_t>(
                    static_cast<std::uint16_t>(p[0]) |
                    (static_cast<std::uint16_t>(p[1])<<8));
                value=static_cast<float>(s/32768.0);
            }
            else if(wav.format==1 && wav.bitsPerSample==24)
            {
                std::int32_t s=static_cast<std::int32_t>(p[0]) |
                               (static_cast<std::int32_t>(p[1])<<8) |
                               (static_cast<std::int32_t>(p[2])<<16);
                if(s&0x00800000)s|=static_cast<std::int32_t>(0xff000000);
                value=static_cast<float>(s/8388608.0);
            }
            else if(wav.format==1 && wav.bitsPerSample==32)
            {
                std::int32_t s{};
                std::memcpy(&s,p,4);
                value=static_cast<float>(s/2147483648.0);
            }
            else return false;

            sum+=std::isfinite(value)?value:0.0f;
        }
        wav.mono[frame]=static_cast<float>(sum/static_cast<double>(wav.channels));
    }

    return true;
}
}

int main(int argc,char** argv)
{
    if(argc!=2)
    {
        std::cerr<<"usage: pmx_polyphonic_probe <mono.wav>\n";
        return 2;
    }

    WavData wav;
    if(!readWav(argv[1],wav))
    {
        std::cerr<<"failed to read wav\n";
        return 3;
    }

    pmx::analysis::PolyphonicNoteDetector detector;
    detector.prepare(static_cast<double>(wav.sampleRate));
    detector.setProfile(pmx::instruments::GuitarProfile::standard());
    detector.reset();

    constexpr int block=128;
    std::uint64_t lastSerial=detector.snapshot().serial;

    for(std::size_t offset=0;offset<wav.mono.size();offset+=block)
    {
        const int count=static_cast<int>(
            std::min<std::size_t>(block,wav.mono.size()-offset));
        detector.process(wav.mono.data()+offset,count);

        const auto snapshot=detector.snapshot();
        if(snapshot.serial==lastSerial)continue;
        lastSerial=snapshot.serial;

        const double processed=static_cast<double>(offset+static_cast<std::size_t>(count));
        const double centerSeconds=std::max(0.0,(processed-2048.0)/static_cast<double>(wav.sampleRate));

        std::cout<<centerSeconds<<","<<snapshot.noteCount;
        for(int i=0;i<snapshot.noteCount;++i)
            std::cout<<","<<snapshot.notes[static_cast<std::size_t>(i)].midiNote
                     <<":"<<snapshot.notes[static_cast<std::size_t>(i)].confidence;
        std::cout<<"\n";
    }

    return 0;
}
