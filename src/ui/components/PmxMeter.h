#pragma once
#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pmx::ui
{
class PmxMeter final : public juce::Component, private juce::Timer
{
public:
    void setLevel(float newLevel) noexcept;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    std::atomic<float> level { 0.0f };
};
} // namespace pmx::ui
