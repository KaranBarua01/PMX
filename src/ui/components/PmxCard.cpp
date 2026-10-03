#include "PmxCard.h"
#include "../PmxTheme.h"

namespace pmx::ui
{
void PmxCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(juce::Colour(Theme::panel));
    g.fillRoundedRectangle(bounds, Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(bounds, Theme::cornerRadius, 1.0f);
}
} // namespace pmx::ui
