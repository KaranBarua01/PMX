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
    close();requestedName=selection.deviceName;requestedRate=selection.sampleRate;requestedBlock=selection.bufferSize;
    juce::AudioIODeviceType* chosen=nullptr;
    for(auto* type:manager.getAvailableDeviceTypes()){
        if(!type->getTypeName().containsIgnoreCase("ASIO"))continue;
        type->scanForDevices();
        if(type->getDeviceNames().contains(juce::String(selection.deviceName))){chosen=type;break;}
    }
    if(!chosen)return "Pocket Master ASIO could not be found. Connect it by USB and check that its ASIO driver is installed.";
    device.reset(chosen->createDevice(juce::String(selection.deviceName),juce::String(selection.deviceName)));
    if(!device)return "Pocket Master could not be created. Close other audio applications and try again.";
    juce::BigInteger inputs,outputs;inputs.setBit(selection.inputChannel);outputs.setBit(selection.outputLeft);outputs.setBit(selection.outputRight);
    const auto error=device->open(inputs,outputs,selection.sampleRate,selection.bufferSize);
    if(error.isNotEmpty()){device.reset();return "Pocket Master could not be opened. Another application may be using its ASIO driver. Close other audio apps and try again. Details: "+error;}
    startAuthorised.store(true);device->start(this);callbackAttached=true;
    if(!deviceAlive.load()){close();return "The ASIO driver opened a different device or changed the requested audio settings. Output remains muted.";}
    return {};
}
void JuceAudioHost::close()
{
    maintenance.store(false);startAuthorised.store(false);
    if(device){device->stop();device->close();device.reset();}
    callbackAttached=false;deviceAlive.store(false);
}
void JuceAudioHost::suspend()
{
    maintenance.store(true);
    if(device&&callbackAttached){device->stop();callbackAttached=false;}
}
bool JuceAudioHost::resume()
{
    if(!device||!device->isOpen()||device->getCurrentSampleRate()!=preparedRate||device->getCurrentBufferSizeSamples()!=preparedBlock){maintenance.store(false);deviceAlive.store(false);return false;}
    startAuthorised.store(true);device->start(this);callbackAttached=true;maintenance.store(false);return deviceAlive.load();
}

void JuceAudioHost::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if(!startAuthorised.exchange(false) || !device || !device->getTypeName().containsIgnoreCase("ASIO") || device->getName().toStdString()!=requestedName || device->getCurrentSampleRate()!=requestedRate || device->getCurrentBufferSizeSamples()!=requestedBlock)
    {deviceAlive.store(false);return;}
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
