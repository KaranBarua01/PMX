#include "app/RuntimePolicy.h"

int main()
{
    using namespace pmx::audio;
    std::vector<AudioDeviceInfo> devices {
        {"Laptop Speakers", false, 0, 2, {48000.0}, {256}},
        {"Pocket Master ASIO", true, 2, 2, {48000.0, 44100.0}, {64, 128, 256}}
    };
    const auto preferred = pmx::app::RuntimePolicy::preferredPocketMaster(devices);
    if (!preferred) return 1;
    if (preferred->deviceName != "Pocket Master ASIO") return 2;
    if (preferred->sampleRate != 44100.0) return 3;
    if (preferred->bufferSize != 128) return 4;
    if (preferred->inputChannel != 0 || preferred->outputLeft != 0 || preferred->outputRight != 1) return 5;

    devices[1].sampleRates = {48000.0};
    devices[1].bufferSizes = {256};
    const auto fallback = pmx::app::RuntimePolicy::preferredPocketMaster(devices);
    if (!fallback || fallback->sampleRate != 48000.0 || fallback->bufferSize != 256) return 6;

    devices[1] = {"Sonicake USB Audio Device", true, 2, 2, {44100.0, 48000.0}, {128, 256}};
    const auto sonicake = pmx::app::RuntimePolicy::preferredPocketMaster(devices);
    if (!sonicake || sonicake->deviceName != "Sonicake USB Audio Device") return 7;

    devices.erase(devices.begin() + 1);
    if (pmx::app::RuntimePolicy::preferredPocketMaster(devices)) return 8;
    return 0;
}
