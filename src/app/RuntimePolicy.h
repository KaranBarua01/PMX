#pragma once
#include <optional>
#include <vector>
#include "audio/AudioStatus.h"

namespace pmx::app
{
class RuntimePolicy final
{
public:
    static std::optional<audio::AudioDeviceSelection>
    preferredPocketMaster(const std::vector<audio::AudioDeviceInfo>& devices);
};
} // namespace pmx::app
