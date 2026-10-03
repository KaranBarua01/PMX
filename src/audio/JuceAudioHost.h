#pragma once
#include <atomic>
#include <juce_audio_devices/juce_audio_devices.h>
#include "AudioDeviceController.h"
#include "AudioSink.h"

namespace pmx::audio
{
class JuceAudioHost final : private juce::AudioIODeviceCallback
{
public:
    JuceAudioHost();
    ~JuceAudioHost() override;

    [[nodiscard]] std::vector<AudioDeviceInfo> scan();
    [[nodiscard]] juce::String open(const AudioDeviceSelection& selection);
    void close();
    void setSink(AudioSink* newSink) noexcept { sink.store(newSink, std::memory_order_release); }
    [[nodiscard]] juce::AudioDeviceManager& deviceManager() noexcept { return manager; }

private:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels,
                                          int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager manager;
    std::atomic<AudioSink*> sink { nullptr };
    bool callbackAttached { false };
};
} // namespace pmx::audio
