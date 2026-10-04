#pragma once
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "state/SetupState.h"
#include "ui/components/PmxButton.h"

namespace pmx::ui
{
class FirstRunWizard final : public juce::Component
{
public:
    FirstRunWizard();
    void paint(juce::Graphics&) override;
    void resized() override;

    void setDetectedStatus(bool inputOk, bool outputOk, bool asioOk);
    void setAudioTestPassed(bool passed);
    std::function<void()> onDiscoverDevice;
    std::function<void()> onTestAudio;
    std::function<void()> onFinished;

private:
    void refresh();
    void continuePressed();

    state::SetupState setup;
    juce::Label stepIndicator;
    juce::Label icon;
    juce::Label title;
    juce::Label subtitle;
    juce::Label deviceSummary;
    juce::Label hint;
    PmxButton primary { "CONTINUE", ButtonKind::primary };
    PmxButton secondary { "", ButtonKind::secondary };
    PmxButton offline { "EXPLORE OFFLINE" };
    juce::Rectangle<int> card;
};
} // namespace pmx::ui
