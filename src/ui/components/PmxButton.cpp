#include "PmxButton.h"
#include "../PmxTheme.h"

namespace pmx::ui
{
namespace { juce::Colour c(std::uint32_t argb) { return juce::Colour(argb); } }

PmxButton::PmxButton(juce::String text, ButtonKind kind) : juce::TextButton(std::move(text))
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setColour(textColourOffId, c(Theme::text));
    setColour(textColourOnId, c(Theme::text));
    auto base = c(Theme::panelRaised);
    if (kind == ButtonKind::primary) base = c(Theme::accent);
    if (kind == ButtonKind::healthy) base = c(Theme::healthy).darker(0.35f);
    if (kind == ButtonKind::danger) base = c(Theme::danger).darker(0.35f);
    setColour(buttonColourId, base);
    setColour(buttonOnColourId, base.brighter(0.1f));
}
} // namespace pmx::ui
