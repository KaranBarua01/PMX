#include "JuceAudioHost.h"
#include "SafeAudioCallback.h"

namespace pmx::audio
{
JuceAudioHost::JuceAudioHost() = default;
JuceAudioHost::~JuceAudioHost() { close(); }

std::vector<AudioDeviceInfo> JuceAudioHost::scan()
{
    std::vector<AudioDeviceInfo> result;
    for (auto* type : manager.getAvailableDeviceTypes())
    {
        if (! type->getTypeName().containsIgnoreCase("ASIO")) continue;
        type->scanForDevices();
        auto inputs = type->getDeviceNames(true);
        auto outputs = type->getDeviceNames(false);
        juce::StringArray names = outputs;
        names.addArray(inputs);
        names.removeDuplicates(false);
        for (const auto& name : names)
        {
            std::unique_ptr<juce::AudioIODevice> device(type->createDevice(name, name));
            if (! device) continue;
            AudioDeviceInfo info;
            info.name = name.toStdString();
            info.asio = true;
            info.inputChannels = device->getInputChannelNames().size();
            info.outputChannels = device->getOutputChannelNames().size();
            for (auto rate : device->getAvailableSampleRates()) info.sampleRates.push_back(rate);
            for (auto size : device->getAvailableBufferSizes()) info.bufferSizes.push_back(size);
            result.push_back(std::move(info));
        }
    }
    return result;
}

juce::String JuceAudioHost::open(const AudioDeviceSelection& selection)
{
    close();
    juce::String asioType;
    for (auto* type : manager.getAvailableDeviceTypes())
        if (type->getTypeName().containsIgnoreCase("ASIO")) { asioType = type->getTypeName(); break; }
    if (asioType.isEmpty()) return "ASIO is not available.";

    manager.setCurrentAudioDeviceType(asioType, true);

    auto setup = manager.getAudioDeviceSetup();
    setup.inputDeviceName = selection.deviceName;
    setup.outputDeviceName = selection.deviceName;
    setup.sampleRate = selection.sampleRate;
    setup.bufferSize = selection.bufferSize;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.clear();
    setup.outputChannels.clear();
    setup.inputChannels.setBit(selection.inputChannel);
    setup.outputChannels.setBit(selection.outputLeft);
    setup.outputChannels.setBit(selection.outputRight);

    const auto error = manager.setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty()) return error;
    manager.addAudioCallback(this);
    callbackAttached = true;
    return {};
}

void JuceAudioHost::close()
{
    maintenance.store(false);
    if (callbackAttached)
    {
        manager.removeAudioCallback(this);
        callbackAttached = false;
    }
    manager.closeAudioDevice();
    deviceAlive.store(false);
}

void JuceAudioHost::suspend()
{
    maintenance.store(true);
    if(callbackAttached){manager.removeAudioCallback(this);callbackAttached=false;}
}

bool JuceAudioHost::resume()
{
    auto* device=manager.getCurrentAudioDevice();
    if(!device || !device->isOpen() || device->getCurrentSampleRate()!=preparedRate || device->getCurrentBufferSizeSamples()!=preparedBlock)
    { maintenance.store(false);deviceAlive.store(false);return false; }
    manager.addAudioCallback(this);callbackAttached=true;maintenance.store(false);return true;
}

void JuceAudioHost::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    deviceAlive.store(true);
    if(maintenance.load())return;
    preparedRate=device->getCurrentSampleRate();preparedBlock=device->getCurrentBufferSizeSamples();
    if (auto* s = sink.load(std::memory_order_acquire))
        s->prepare(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples(),
                   device->getActiveInputChannels().countNumberOfSetBits(),
                   device->getActiveOutputChannels().countNumberOfSetBits());
}

void JuceAudioHost::audioDeviceStopped()
{
    if(maintenance.load())return;
    deviceAlive.store(false);
    if (auto* s = sink.load(std::memory_order_acquire)) s->stopped();
}

void JuceAudioHost::audioDeviceIOCallbackWithContext(const float* const* inputs, int numInputs,
                                                      float* const* outputs, int numOutputs,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext&)
{
    if (auto* s = sink.load(std::memory_order_acquire);s && deviceAlive.load())
        s->process(inputs, numInputs, outputs, numOutputs, numSamples);
    else
        for (int c = 0; c < numOutputs; ++c)
            if (outputs[c] != nullptr) juce::FloatVectorOperations::clear(outputs[c], numSamples);

    SafeAudioCallback::sanitize(outputs, numOutputs, numSamples);
}
} // namespace pmx::audio
