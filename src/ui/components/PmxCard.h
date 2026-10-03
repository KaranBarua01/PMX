#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace pmx::ui
{
class PmxCard : public juce::Component
{
public:
    void paint(juce::Graphics&) override;
};
} // namespace pmx::ui
