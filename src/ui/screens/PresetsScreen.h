#pragma once
#include <functional>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"
#include "presets/SoundState.h"
namespace pmx::ui {
class PresetsScreen final:public juce::Component {
public:
 PresetsScreen();void paint(juce::Graphics&) override;void resized() override;
 void setPresets(std::vector<presets::Preset>);void setCurrent(const std::string&);
 std::function<void()> onSavePreset;
 std::function<void(const std::string&)> onPresetChosen;
 std::function<void(const std::string&,bool)> onFavoriteChanged;
private:
 void rebuild();
 juce::Label title,subtitle,empty;juce::TextEditor search;
 PmxButton savePreset{"SAVE PRESET",ButtonKind::primary};
 std::array<PmxButton,8> categories{PmxButton{"All sounds"},PmxButton{"Acoustic"},PmxButton{"Clean"},PmxButton{"Blues"},PmxButton{"Rock"},PmxButton{"Metal"},PmxButton{"Special"},PmxButton{"Favorites"}};
 std::vector<presets::Preset> library;std::vector<std::unique_ptr<PmxButton>> cards,stars;
 juce::Viewport viewport;juce::Component grid;
 juce::String category{"All sounds"};std::string currentId{"factory-4"};
};}

