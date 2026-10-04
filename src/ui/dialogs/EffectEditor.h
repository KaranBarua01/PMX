#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxKnob.h"
#include "ui/components/PmxButton.h"
#include "presets/SoundState.h"
namespace pmx::ui {
class EffectEditor final: public juce::Component {
public:
 EffectEditor();void paint(juce::Graphics&) override;void resized() override;
 void showEffect(std::size_t,const presets::EffectSettings&);
 std::function<void()> onDone,onTapTempo;
 std::function<void(std::size_t,const presets::EffectSettings&)> onChanged;
 float timeMs() const noexcept{return values[0];}float feedbackPercent() const noexcept{return values[1];}float mixPercent() const noexcept{return values[2];}
private:
 void notify();std::size_t effectIndex{7};std::array<float,3> values{420,32,24};bool updating{false};
 juce::Rectangle<int> card;juce::Label eyebrow,title,typeLabel;std::array<juce::Label,3> labels;std::array<PmxKnob,3> knobs;
 PmxButton enabled{"ON"},close{"CLOSE"},tap{"TAP TEMPO"},done{"DONE",ButtonKind::primary};
};}

