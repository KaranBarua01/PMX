#include "PmxMeter.h"
#include "../PmxTheme.h"
#include <algorithm>

namespace pmx::ui
{
void PmxMeter::setLevel(float newLevel) noexcept
{
    level.store(std::clamp(newLevel, 0.0f, 1.0f), std::memory_order_relaxed);
    if (! isTimerRunning()) startTimerHz(30);
}

void PmxMeter::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colour(Theme::panelRaised));
    g.fillRoundedRectangle(r, 3.0f);
    auto fill = r.withWidth(r.getWidth() * level.load(std::memory_order_relaxed));
    g.setColour(juce::Colour(Theme::accent));
    g.fillRoundedRectangle(fill, 3.0f);
}
} // namespace pmx::ui
