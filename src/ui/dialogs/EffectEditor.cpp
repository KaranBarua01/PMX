#include "EffectEditor.h"
#include "ui/PmxTheme.h"
namespace pmx::ui {
EffectEditor::EffectEditor() {
 eyebrow.setText("EFFECT EDITOR",juce::dontSendNotification);eyebrow.setColour(juce::Label::textColourId,juce::Colour(Theme::accent));
 title.setFont(juce::FontOptions(28.0f,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
 typeLabel.setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));
 for(auto* c:std::initializer_list<juce::Component*>{&eyebrow,&title,&typeLabel,&enabled,&close,&tap,&done})addAndMakeVisible(*c);
 for(std::size_t i=0;i<3;++i){addAndMakeVisible(knobs[i]);addAndMakeVisible(labels[i]);labels[i].setJustificationType(juce::Justification::centred);labels[i].setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));knobs[i].onValueChange=[this,i]{values[i]=static_cast<float>(knobs[i].getValue());notify();};}
 enabled.setClickingTogglesState(true);enabled.onClick=[this]{enabled.setButtonText(enabled.getToggleState()?"ON":"OFF");notify();};
 close.onClick=done.onClick=[this]{if(onDone)onDone();};tap.onClick=[this]{if(onTapTempo)onTapTempo();};
 showEffect(7,presets::SoundState{}.effects[7]);
}
void EffectEditor::notify(){if(!updating&&onChanged)onChanged(effectIndex,{enabled.getToggleState(),values});}
void EffectEditor::showEffect(std::size_t index,const presets::EffectSettings& state) {
 updating=true;effectIndex=index;values=state.values;const auto& spec=presets::effectSpecs[index];
 title.setText(spec.name,juce::dontSendNotification);typeLabel.setText(spec.description,juce::dontSendNotification);
 enabled.setToggleState(state.enabled,juce::dontSendNotification);enabled.setButtonText(state.enabled?"ON":"OFF");
 for(std::size_t i=0;i<3;++i){const auto& p=spec.parameters[i];knobs[i].setVisible(static_cast<int>(i)<spec.parameterCount);labels[i].setVisible(knobs[i].isVisible());knobs[i].setRange(p.minimum,p.maximum,p.step);knobs[i].setTextValueSuffix(p.unit);knobs[i].setValue(values[i],juce::dontSendNotification);labels[i].setText(p.title,juce::dontSendNotification);}
 tap.setVisible(index==7);updating=false;resized();repaint();
}
void EffectEditor::paint(juce::Graphics& g){g.fillAll(juce::Colours::black.withAlpha(.72f));g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(card.toFloat(),12);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(card.toFloat().reduced(.5f),12,1);}
void EffectEditor::resized(){
 card=getLocalBounds().withSizeKeepingCentre(juce::jmin(520,juce::jmax(300,getWidth()-40)),400);
 auto r=card.reduced(26);auto header=r.removeFromTop(24);close.setBounds(header.removeFromRight(68));enabled.setBounds(header.removeFromRight(54).reduced(4,0));eyebrow.setBounds(header);
 title.setBounds(r.removeFromTop(44));typeLabel.setBounds(r.removeFromTop(38));r.removeFromTop(16);
 auto row=r.removeFromTop(156);int count=presets::effectSpecs[effectIndex].parameterCount;int width=row.getWidth()/count;
 for(int i=0;i<count;++i){auto k=row.removeFromLeft(width);labels[static_cast<std::size_t>(i)].setBounds(k.removeFromBottom(24));knobs[static_cast<std::size_t>(i)].setBounds(k.reduced(10,0));}
 auto bottom=r.removeFromBottom(38);tap.setBounds(bottom.removeFromLeft(124));done.setBounds(bottom.removeFromRight(100));
}
}

