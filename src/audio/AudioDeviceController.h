#pragma once
#include <vector>
#include "AudioStatus.h"

namespace pmx::audio
{
class AudioDeviceController final
{
public:
    void updateAvailableDevices(std::vector<AudioDeviceInfo> devices);
    [[nodiscard]] const std::vector<AudioDeviceInfo>& availableDevices() const noexcept { return devices; }
    [[nodiscard]] bool open(const AudioDeviceSelection& selection);
    void close() noexcept;
    void notifyDisconnected() noexcept;
    [[nodiscard]] bool setMonitoringEnabled(bool enabled) noexcept;
    [[nodiscard]] const AudioStatus& status() const noexcept { return currentStatus; }
    [[nodiscard]] const AudioDeviceSelection& selection() const noexcept { return currentSelection; }

private:
    std::vector<AudioDeviceInfo> devices;
    AudioDeviceSelection currentSelection;
    AudioStatus currentStatus;
};
} // namespace pmx::audio
