#include "LiveScreen.h"
#include "ui/PmxTheme.h"
namespace pmx::ui
{
LiveScreen::LiveScreen()
{
    eyebrow.setText("YOUR SOUND • GUITAR",juce::dontSendNotification); eyebrow.setColour(juce::Label::textColourId,juce::Colour(Theme::accent));
    title.setText("DREAM CLEAN",juce::dontSendNotification); title.setFont(juce::FontOptions(34.0f,juce::Font::bold)); title.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
    subtitle.setText("Wide, glassy and alive — built to stay out of your way.",juce::dontSendNotification); subtitle.setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));
    status.setText("● POCKET MASTER CONNECTED    48 kHz • 128 samples",juce::dontSendNotification); status.setJustificationType(juce::Justification::centredRight); status.setColour(juce::Label::textColourId,juce::Colour(Theme::healthy));
    for(auto* c:{static_cast<juce::Component*>(&eyebrow),&title,&subtitle,&status,&tuner,&chooseSound,&perform,&save,&bypass,&namCard,&irCard,&tunerView,&effectEditor,&namIrBrowser}) addAndMakeVisible(*c);
    tunerView.setVisible(false); effectEditor.setVisible(false); namIrBrowser.setVisible(false);
    tuner.onClick=[this]{toggleTuner();};
    save.onClick=[this]{if(onSavePreset)onSavePreset();};
    bypass.setClickingTogglesState(true);
    bypass.onClick=[this]{if(onBypassChanged)onBypassChanged(bypass.getToggleState());};
    chooseSound.onClick=[this]{if(onChooseSound)onChooseSound();};
    perform.onClick=[this]{if(onPerformanceMode)onPerformanceMode();};
    modules[7].onClick=[this]{effectEditor.setVisible(true);effectEditor.toFront(false);resized();};
    modules[3].onClick=[this]{namIrBrowser.setVisible(true);namIrBrowser.toFront(false);resized();};
    modules[4].onClick=[this]{namIrBrowser.setVisible(true);namIrBrowser.toFront(false);resized();};
    namCard.onClick=[this]{namIrBrowser.setVisible(true);namIrBrowser.toFront(false);resized();};
    irCard.onClick=[this]{namIrBrowser.setVisible(true);namIrBrowser.toFront(false);resized();};
    effectEditor.onTapTempo=[this]{if(onTapTempo)onTapTempo();};
    effectEditor.onDone=[this]{if(onDelayChanged)onDelayChanged(effectEditor.timeMs(),effectEditor.feedbackPercent(),effectEditor.mixPercent());effectEditor.setVisible(false);};
    namIrBrowser.onImportNam=[this]{if(onImportNam)onImportNam();};
    namIrBrowser.onImportIr=[this]{if(onImportIr)onImportIr();};
    namIrBrowser.onLoadSelected=[this]{namIrBrowser.setVisible(false);};
    for(auto& m:modules)addAndMakeVisible(m);
}
void LiveScreen::toggleTuner(){tunerView.setVisible(!tunerView.isVisible());resized();}
void LiveScreen::setBypassVisual(bool enabled){bypass.setToggleState(enabled,juce::dontSendNotification);}
void LiveScreen::setStatusText(const std::string& text, bool healthy){status.setText(juce::String(text),juce::dontSendNotification);status.setColour(juce::Label::textColourId,juce::Colour(healthy?Theme::healthy:Theme::danger));}
void LiveScreen::setPresetName(const std::string& name){title.setText(juce::String(name),juce::dontSendNotification);}
void LiveScreen::setNamName(const std::string& name){namCard.setButtonText(juce::String(name));}
void LiveScreen::setIrName(const std::string& name){irCard.setButtonText(juce::String(name));}
void LiveScreen::paint(juce::Graphics& g){ g.fillAll(juce::Colour(Theme::background)); g.setColour(juce::Colour(Theme::mutedText)); g.setFont(12.0f); g.drawText("YOUR SIGNAL CHAIN",24,170,180,20,juce::Justification::centredLeft); g.drawText("AMP MODEL",24,310,180,20,juce::Justification::centredLeft); g.drawText("CABINET IR",getWidth()/2+12,310,180,20,juce::Justification::centredLeft); }
void LiveScreen::resized(){ auto r=getLocalBounds().reduced(24); auto top=r.removeFromTop(130); status.setBounds(top.removeFromRight(310).removeFromTop(28)); auto actions=top.removeFromRight(250).removeFromBottom(38); save.setBounds(actions.removeFromRight(130)); bypass.setBounds(actions.removeFromRight(100).reduced(4,0)); tuner.setBounds(top.removeFromRight(90).removeFromBottom(38)); if(tunerView.isVisible()) tunerView.setBounds(getWidth()-360,76,320,72);
    auto secondary=top.removeFromRight(220).removeFromBottom(38); perform.setBounds(secondary.removeFromRight(90)); chooseSound.setBounds(secondary.removeFromRight(125).reduced(4,0));
    eyebrow.setBounds(top.removeFromTop(24)); title.setBounds(top.removeFromTop(46)); subtitle.setBounds(top.removeFromTop(30)); r.removeFromTop(34); auto chain=r.removeFromTop(100); const int w=chain.getWidth()/9; for(auto& m:modules)m.setBounds(chain.removeFromLeft(w).reduced(5,12)); r.removeFromTop(44); auto assets=r.removeFromTop(80); auto left=assets.removeFromLeft(assets.getWidth()/2).reduced(5); namCard.setBounds(left); irCard.setBounds(assets.reduced(5));
    if(effectEditor.isVisible()) effectEditor.setBounds((getWidth()-500)/2,(getHeight()-390)/2,500,390);
    if(namIrBrowser.isVisible()) namIrBrowser.setBounds((getWidth()-680)/2,(getHeight()-470)/2,680,470);
}
}
