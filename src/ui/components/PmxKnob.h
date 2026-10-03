#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace pmx::ui
{
class PmxKnob final : public juce::Slider
{
public:
    explicit PmxKnob(juce::String suffix = {});
};
} // namespace pmx::ui
