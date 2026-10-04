#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "audio/AudioStatus.h"
#include "ui/components/PmxButton.h"

namespace pmx::ui
{
class SettingsScreen final : public juce::Component
{
public:
    SettingsScreen();
    void paint(juce::Graphics&) override;
    void resized() override;

    void setAvailableDevices(const std::vector<audio::AudioDeviceInfo>& devices);
    void setSelection(const audio::AudioDeviceSelection&);
    [[nodiscard]] audio::AudioDeviceSelection selectedAudioSetup() const;
    void setConnectionStatus(bool connected, const std::string& connectionText,
                             const std::string& detail, double driverLatencyMs);
    [[nodiscard]] float inputGainDb() const noexcept { return static_cast<float>(inputGain.getValue()); }
    [[nodiscard]] float outputGainDb() const noexcept { return static_cast<float>(outputGain.getValue()); }

    std::function<void()> onApply;
    std::function<void()> onTestAudio;
    std::function<void()> onTryAgain;

private:
    void refreshChannels();
    std::vector<audio::AudioDeviceInfo> available;
    std::vector<double> rates;
    std::vector<int> buffers;
    std::string detailText;
    juce::Rectangle<int> connectionCard,stabilityCard,gainCard,latencyCard,helpCard;
    juce::Label title, subtitle, connectionTitle, connectionState, latencyTitle, latencyValue;
    juce::Label stabilityTitle, inputGainLabel, outputGainLabel, helpTitle, helpText;
    juce::ComboBox device, inputChannel, outputPair, sampleRate, bufferSize;
    juce::Slider inputGain, outputGain;
    PmxButton apply { "APPLY CHANGES", ButtonKind::primary };
    PmxButton testAudio { "TEST AUDIO" };
    PmxButton tryAgain { "TRY AGAIN", ButtonKind::primary };
    PmxButton showDetails { "SHOW DETAILS" };
};
} // namespace pmx::ui
