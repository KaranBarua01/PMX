#pragma once
#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
#include "ui/components/PmxMeter.h"
#include "ui/components/PmxKnob.h"

namespace pmx::ui
{
class PerformanceMode final : public juce::Component
{
public:
    PerformanceMode();
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void()> onExit;

private:
    juce::Label title, subtitle, preset, connection, shortcutHint;
    PmxButton exit { "EXIT PERFORMANCE" };
    PmxButton tuner { "TUNER" };
    PmxButton bypass { "BYPASS", ButtonKind::primary };
    PmxButton mute { "MUTE", ButtonKind::danger };
    PmxButton loop { "LOOP  •  PLAY" };
    PmxButton quickRecord { "QUICK RECORD" };
    PmxButton tap { "TAP 120 BPM" };
    std::array<PmxButton,6> chain { PmxButton{"COMP"},PmxButton{"DRIVE"},PmxButton{"NAM"},PmxButton{"IR"},PmxButton{"DELAY"},PmxButton{"REVERB"} };
    PmxKnob delayTime { " ms" }, feedback { " %" }, mix { " %" };
    PmxMeter inputMeter, outputMeter;
};
} // namespace pmx::ui
