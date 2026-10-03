#include "PmxKnob.h"
#include "../PmxTheme.h"

namespace pmx::ui
{
PmxKnob::PmxKnob(juce::String suffix)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 22);
    setTextValueSuffix(std::move(suffix));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Theme::accent));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Theme::border));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(Theme::text));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
} // namespace pmx::ui
