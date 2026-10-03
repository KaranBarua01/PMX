#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace pmx::ui
{
enum class ButtonKind { secondary, primary, healthy, danger };

class PmxButton final : public juce::TextButton
{
public:
    PmxButton(juce::String text, ButtonKind kind = ButtonKind::secondary);
};
} // namespace pmx::ui
