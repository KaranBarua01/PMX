#pragma once
#include <array>
#include <functional>
#include <string>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"

namespace pmx::ui
{
class PresetsScreen final : public juce::Component
{
public:
    PresetsScreen();
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void()> onSavePreset;
    std::function<void(const std::string&)> onPresetChosen;

private:
    juce::Label title;
    juce::Label subtitle;
    juce::TextEditor search;
    PmxButton savePreset { "SAVE PRESET", ButtonKind::primary };
    std::array<PmxButton, 8> categories {
        PmxButton{"All sounds"}, PmxButton{"Acoustic"}, PmxButton{"Clean"}, PmxButton{"Blues"},
        PmxButton{"Rock"}, PmxButton{"Metal"}, PmxButton{"Special"}, PmxButton{"Favorites"}
    };
    std::array<PmxButton, 12> cards {
        PmxButton{"Natural Acoustic"}, PmxButton{"Acoustic Space"}, PmxButton{"Glass Clean"}, PmxButton{"80s Clean"},
        PmxButton{"Dream Clean"}, PmxButton{"Dumble Warm"}, PmxButton{"Blues Lead"}, PmxButton{"Classic Rock"},
        PmxButton{"Arena Rock"}, PmxButton{"Hard Rock"}, PmxButton{"Tight Metal"}, PmxButton{"Metal Lead"}
    };
};
} // namespace pmx::ui
