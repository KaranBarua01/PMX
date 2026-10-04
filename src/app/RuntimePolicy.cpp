#include "RuntimePolicy.h"
#include <algorithm>
#include <cctype>

namespace pmx::app
{
namespace
{
bool containsInsensitive(const std::string& text, const std::string& needle)
{
    auto lower=[](unsigned char c){return static_cast<char>(std::tolower(c));};
    std::string a(text.size(),' '), b(needle.size(),' ');
    std::transform(text.begin(),text.end(),a.begin(),lower);
    std::transform(needle.begin(),needle.end(),b.begin(),lower);
    return a.find(b)!=std::string::npos;
}

template <typename T>
T choose(const std::vector<T>& values, std::initializer_list<T> preferences)
{
    for(const auto preferred:preferences)
        if(std::find(values.begin(),values.end(),preferred)!=values.end()) return preferred;
    return values.empty()?T{}:values.front();
}
}

std::optional<audio::AudioDeviceSelection>
RuntimePolicy::preferredPocketMaster(const std::vector<audio::AudioDeviceInfo>& devices)
{
    const auto it=std::find_if(devices.begin(),devices.end(),[](const auto& d){
        const bool pocketMasterName = containsInsensitive(d.name,"Pocket Master");
        const bool sonicakeWindowsDriver = containsInsensitive(d.name,"Sonicake USB Audio Device");
        return d.asio && d.inputChannels>0 && d.outputChannels>=2
            && (pocketMasterName || sonicakeWindowsDriver);
    });
    if(it==devices.end()) return std::nullopt;
    audio::AudioDeviceSelection out;
    out.deviceName=it->name;
    out.sampleRate=choose<double>(it->sampleRates,{44100.0,48000.0});
    out.bufferSize=choose<int>(it->bufferSizes,{128,64,256});
    out.inputChannel=0;
    out.outputLeft=0;
    out.outputRight=1;
    if(out.sampleRate<=0.0 || out.bufferSize<=0) return std::nullopt;
    return out;
}
} // namespace pmx::app
