#include <string>
#include <vector>
#include "audio/AudioDeviceController.h"

int main()
{
    using namespace pmx::audio;
    AudioDeviceController controller;
    controller.updateAvailableDevices({
        AudioDeviceInfo{"Laptop Mic", false, 1, 0, {44100.0}, {128}},
        AudioDeviceInfo{"Pocket Master ASIO", true, 2, 2, {44100.0, 48000.0}, {64, 128, 256}}
    });

    AudioDeviceSelection missing{"Not Here", 44100.0, 128, 0, 0, 1};
    if (controller.open(missing)) return 1;
    if (controller.status().connection != DeviceConnection::error) return 2;
    if (! controller.status().deviceName.empty()) return 3;

    AudioDeviceSelection wrongType{"Laptop Mic", 44100.0, 128, 0, 0, 0};
    if (controller.open(wrongType)) return 4;

    AudioDeviceSelection unsupported{"Pocket Master ASIO", 96000.0, 128, 0, 0, 1};
    if (controller.open(unsupported)) return 5;

    AudioDeviceSelection valid{"Pocket Master ASIO", 44100.0, 128, 0, 0, 1};
    if (! controller.open(valid)) return 6;
    if (controller.status().connection != DeviceConnection::open) return 7;
    if (controller.status().monitoringEnabled) return 8;
    if (! controller.setMonitoringEnabled(true)) return 9;
    if (! controller.status().monitoringEnabled) return 10;

    controller.notifyDisconnected();
    if (controller.status().connection != DeviceConnection::disconnected) return 11;
    if (controller.status().monitoringEnabled) return 12;
    if (controller.status().deviceName != "Pocket Master ASIO") return 13;
    return 0;
}
