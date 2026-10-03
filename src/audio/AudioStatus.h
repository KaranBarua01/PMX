#pragma once
#include <string>
#include <vector>

namespace pmx::audio
{
enum class DeviceConnection { closed, open, disconnected, error };

struct AudioDeviceInfo
{
    std::string name;
    bool asio { false };
    int inputChannels { 0 };
    int outputChannels { 0 };
    std::vector<double> sampleRates;
    std::vector<int> bufferSizes;
};

struct AudioDeviceSelection
{
    std::string deviceName;
    double sampleRate { 44100.0 };
    int bufferSize { 128 };
    int inputChannel { 0 };
    int outputLeft { 0 };
    int outputRight { 1 };
};

struct AudioStatus
{
    DeviceConnection connection { DeviceConnection::closed };
    std::string deviceName;
    std::string message;
    double sampleRate { 0.0 };
    int bufferSize { 0 };
    bool monitoringEnabled { false };
};
} // namespace pmx::audio
