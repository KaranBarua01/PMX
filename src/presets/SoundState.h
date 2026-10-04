#pragma once
#include "Preset.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
namespace pmx::presets
{
struct ParameterSpec
{
    const char* key; const char* title; const char* unit;
    float minimum, maximum, initial, step;
};
struct EffectSpec
{
    const char* name; const char* description;
    int parameterCount;
    std::array<ParameterSpec,3> parameters;
};
inline constexpr std::array<EffectSpec,9> effectSpecs{{
    {"GATE","Quiet between notes",3,{{{"gate.thresholdDb","THRESHOLD"," dB",-90,0,-60,1},{"gate.attackMs","ATTACK"," ms",0.1f,50,3,0.1f},{"gate.releaseMs","RELEASE"," ms",10,1000,120,1}}}},
    {"COMP","Smooth & balanced",3,{{{"comp.thresholdDb","THRESHOLD"," dB",-60,0,-18,1},{"comp.ratio","RATIO",":1",1,20,4,0.1f},{"comp.makeupDb","MAKEUP"," dB",0,18,0,0.1f}}}},
    {"DRIVE","Soft overdrive",3,{{{"drive.amountPercent","DRIVE"," %",0,100,25,1},{"drive.tonePercent","TONE"," %",0,100,50,1},{"drive.outputDb","OUTPUT"," dB",-24,6,0,0.1f}}}},
    {"NAM","Your local amp capture",2,{{{"nam.inputTrimDb","INPUT TRIM"," dB",-24,24,0,0.1f},{"nam.outputTrimDb","OUTPUT TRIM"," dB",-24,6,0,0.1f},{"","","",0,1,0,1}}}},
    {"IR","Your local cabinet",3,{{{"ir.lowCutHz","LOW CUT"," Hz",20,500,60,1},{"ir.highCutHz","HIGH CUT"," Hz",1000,20000,8000,10},{"ir.outputDb","OUTPUT"," dB",-24,6,0,0.1f}}}},
    {"EQ","Balanced",3,{{{"eq.lowDb","LOW"," dB",-18,18,0,0.1f},{"eq.midDb","MID"," dB",-18,18,0,0.1f},{"eq.highDb","HIGH"," dB",-18,18,0,0.1f}}}},
    {"MOD","Chorus",3,{{{"mod.rateHz","RATE"," Hz",0.05f,8,0.8f,0.05f},{"mod.depthPercent","DEPTH"," %",0,100,35,1},{"mod.mixPercent","MIX"," %",0,100,25,1}}}},
    {"DELAY","Warm repeats",3,{{{"delay.timeMs","TIME"," ms",1,2000,420,1},{"delay.feedbackPercent","FEEDBACK"," %",0,95,32,1},{"delay.mixPercent","MIX"," %",0,100,24,1}}}},
    {"REVERB","Wide hall",3,{{{"reverb.decayPercent","DECAY"," %",0,92,65,1},{"reverb.tonePercent","TONE"," %",0,100,60,1},{"reverb.mixPercent","MIX"," %",0,100,18,1}}}}
}};
inline constexpr std::array<const char*,9> moduleKeys{"Gate","Comp","Drive","NAM","IR","EQ","Mod","Delay","Reverb"};
struct EffectSettings { bool enabled{false}; std::array<float,3> values{}; };
struct SoundState
{
    std::array<EffectSettings,9> effects{};
    AssetReference nam,ir;
    SoundState()
    {
        for(std::size_t i=0;i<effects.size();++i)
        {
            effects[i].enabled=i!=2 && i!=3 && i!=4;
            for(int p=0;p<3;++p) effects[i].values[static_cast<std::size_t>(p)]=effectSpecs[i].parameters[static_cast<std::size_t>(p)].initial;
        }
    }
    [[nodiscard]] Preset toPreset(std::string id,std::string name,std::string category) const
    {
        Preset result; result.id=std::move(id); result.displayName=std::move(name); result.category=std::move(category); result.nam=nam; result.ir=ir;
        for(std::size_t i=0;i<effects.size();++i)
        {
            result.modules[moduleKeys[i]]=effects[i].enabled;
            for(int p=0;p<effectSpecs[i].parameterCount;++p)
                result.parameters[effectSpecs[i].parameters[static_cast<std::size_t>(p)].key]=effects[i].values[static_cast<std::size_t>(p)];
        }
        return result;
    }
    [[nodiscard]] static SoundState fromPreset(const Preset& preset)
    {
        SoundState result; result.nam=preset.nam; result.ir=preset.ir;
        for(std::size_t i=0;i<result.effects.size();++i)
        {
            if(const auto active=preset.modules.find(moduleKeys[i]);active!=preset.modules.end()) result.effects[i].enabled=active->second;
            for(int p=0;p<effectSpecs[i].parameterCount;++p)
            {
                const auto& spec=effectSpecs[i].parameters[static_cast<std::size_t>(p)];
                if(const auto value=preset.parameters.find(spec.key);value!=preset.parameters.end() && std::isfinite(value->second))
                    result.effects[i].values[static_cast<std::size_t>(p)]=static_cast<float>(std::clamp(value->second,static_cast<double>(spec.minimum),static_cast<double>(spec.maximum)));
            }
        }
        return result;
    }
};
inline std::vector<Preset> factoryPresets()
{
    const char* names[]{"Natural Acoustic","Acoustic Space","Glass Clean","80s Clean","Dream Clean","Warm Blues","Blues Lead","Classic Rock","Arena Rock","Hard Rock","Tight Metal","Metal Lead"};
    const char* categories[]{"Acoustic","Acoustic","Clean","Clean","Clean","Blues","Blues","Rock","Rock","Rock","Metal","Metal"};
    std::vector<Preset> result;
    for(int i=0;i<12;++i)
    {
        SoundState s;
        s.effects[6].enabled=i==3||i==4||i==8;
        s.effects[7].enabled=i==1||i==4||i==6||i==8||i==11;
        s.effects[8].values[2]=i==1?30.0f:12.0f;
        s.effects[2].enabled=i>=5;
        s.effects[2].values[0]=i>=10?85.0f:(i>=7?60.0f:25.0f);
        s.effects[2].values[2]=i>=7?-6.0f:0.0f;
        s.effects[5].values={i>=10?-3.0f:0.0f,i>=10?-4.0f:0.0f,i<2?2.0f:0.0f};
        result.push_back(s.toPreset("factory-"+std::to_string(i),names[i],categories[i]));
    }
    return result;
}
}
