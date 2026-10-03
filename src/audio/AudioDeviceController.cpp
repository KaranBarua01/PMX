#include "AudioDeviceController.h"
#include <algorithm>

namespace pmx::audio
{
void AudioDeviceController::updateAvailableDevices(std::vector<AudioDeviceInfo> newDevices)
{
    devices = std::move(newDevices);
}

bool AudioDeviceController::open(const AudioDeviceSelection& requested)
{
    auto it = std::find_if(devices.begin(), devices.end(), [&](const auto& d) { return d.name == requested.deviceName; });
    if (it == devices.end())
    {
        currentStatus = { DeviceConnection::error, {}, "Selected audio device was not found.", 0.0, 0, false };
        return false;
    }
    if (! it->asio)
    {
        currentStatus = { DeviceConnection::error, {}, "PMX requires the Pocket Master ASIO device for this build.", 0.0, 0, false };
        return false;
    }
    const auto rateOk = std::find(it->sampleRates.begin(), it->sampleRates.end(), requested.sampleRate) != it->sampleRates.end();
    const auto bufferOk = std::find(it->bufferSizes.begin(), it->bufferSizes.end(), requested.bufferSize) != it->bufferSizes.end();
    const auto channelsOk = requested.inputChannel >= 0 && requested.inputChannel < it->inputChannels
        && requested.outputLeft >= 0 && requested.outputLeft < it->outputChannels
        && requested.outputRight >= 0 && requested.outputRight < it->outputChannels;
    if (! rateOk || ! bufferOk || ! channelsOk)
    {
        currentStatus = { DeviceConnection::error, {}, "The selected sample rate, buffer, or channel routing is unsupported.", 0.0, 0, false };
        return false;
    }

    currentSelection = requested;
    currentStatus = { DeviceConnection::open, requested.deviceName, "Audio device ready. Monitoring is muted until enabled.", requested.sampleRate, requested.bufferSize, false };
    return true;
}

void AudioDeviceController::close() noexcept
{
    currentStatus = {};
}

void AudioDeviceController::notifyDisconnected() noexcept
{
    if (currentStatus.connection == DeviceConnection::open)
    {
        currentStatus.connection = DeviceConnection::disconnected;
        currentStatus.monitoringEnabled = false;
        currentStatus.message = "Pocket Master disconnected. Output is muted.";
    }
}

bool AudioDeviceController::setMonitoringEnabled(bool enabled) noexcept
{
    if (currentStatus.connection != DeviceConnection::open)
    {
        currentStatus.monitoringEnabled = false;
        return false;
    }
    currentStatus.monitoringEnabled = enabled;
    return true;
}
} // namespace pmx::audio
