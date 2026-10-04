#include "LiveScreen.h"
#include "ui/PmxTheme.h"
#include <cmath>
namespace pmx::ui {
LiveScreen::LiveScreen() {
 for(auto* l:{&eyebrow,&title,&subtitle,&status,&mode,&tempoLabel,&clickLevelLabel}){addAndMakeVisible(*l);l->setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));}
 eyebrow.setText("YOUR SOUND | CLEAN",juce::dontSendNotification);eyebrow.setFont(juce::FontOptions(11.0f,juce::Font::bold));eyebrow.setColour(juce::Label::textColourId,juce::Colour(Theme::accent));
 title.setText("DREAM CLEAN",juce::dontSendNotification);title.setFont(juce::FontOptions(36.0f,juce::Font::bold));title.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
 subtitle.setText("Wide, glassy chords. A little shimmer. Room to breathe.",juce::dontSendNotification);
 mode.setText("GUITAR",juce::dontSendNotification);mode.setColour(juce::Label::textColourId,juce::Colour(Theme::accent));
 status.setText("POCKET MASTER OFFLINE",juce::dontSendNotification);status.setJustificationType(juce::Justification::centredRight);status.setFont(juce::FontOptions(11.0f));
 tempoLabel.setText("BPM",juce::dontSendNotification);clickLevelLabel.setText("CLICK LEVEL",juce::dontSendNotification);
 for(auto* c:std::initializer_list<juce::Component*>{&tuner,&chooseSound,&perform,&save,&bypass,&mute,&record,&click,&tap,&openLooper,&bpm,&clickLevel,&namCard,&irCard,&tunerView,&effectEditor,&namIrBrowser})addAndMakeVisible(*c);
 tunerView.setVisible(false);effectEditor.setVisible(false);namIrBrowser.setVisible(false);
 tuner.onClick=[this]{toggleTuner();};save.onClick=[this]{if(onSavePreset)onSavePreset();};chooseSound.onClick=[this]{if(onChooseSound)onChooseSound();};perform.onClick=[this]{if(onPerformanceMode)onPerformanceMode();};
 bypass.setClickingTogglesState(true);bypass.onClick=[this]{if(onBypassChanged)onBypassChanged(bypass.getToggleState());};
 mute.setClickingTogglesState(true);setMutedVisual(true);mute.onClick=[this]{if(onMuteChanged)onMuteChanged(mute.getToggleState());};
 record.onClick=[this]{if(onQuickRecord)onQuickRecord();};openLooper.onClick=[this]{if(onOpenLooper)onOpenLooper();};
 click.setClickingTogglesState(true);click.onClick=[this]{click.setButtonText(click.getToggleState()?"CLICK ON":"CLICK OFF");if(onMetronomeChanged)onMetronomeChanged(click.getToggleState());};
 tap.onClick=[this]{if(onTapTempo)onTapTempo();};
 bpm.setRange(30,300,1);bpm.setValue(120);bpm.setSliderStyle(juce::Slider::IncDecButtons);bpm.setTextBoxStyle(juce::Slider::TextBoxLeft,false,58,30);bpm.onValueChange=[this]{if(onTempoChanged)onTempoChanged(bpm.getValue());};
 clickLevel.setRange(0,100,1);clickLevel.setValue(12);clickLevel.setSliderStyle(juce::Slider::LinearHorizontal);clickLevel.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);clickLevel.onValueChange=[this]{if(onMetronomeLevel)onMetronomeLevel(static_cast<float>(clickLevel.getValue()/100));};
 for(std::size_t i=0;i<modules.size();++i){addAndMakeVisible(modules[i]);modules[i].getProperties().set("subtitle",presets::effectSpecs[i].description);modules[i].onClick=[this,i]{openEditor(i);};}
 namCard.getProperties().set("category","AMP CAPTURE | NAM");namCard.getProperties().set("subtitle","Choose a local .nam file");
 irCard.getProperties().set("category","CABINET | IR");irCard.getProperties().set("subtitle","Choose a local WAV impulse");
 namCard.onClick=irCard.onClick=[this]{openLibrary();};
 effectEditor.onChanged=[this](std::size_t i,const presets::EffectSettings& s){sound.effects[i]=s;modules[i].getProperties().set("active",s.enabled);modules[i].repaint();if(onEffectChanged)onEffectChanged(i,s);};
 effectEditor.onDone=[this]{effectEditor.setVisible(false);for(auto& m:modules)m.setToggleState(false,juce::dontSendNotification);};
 effectEditor.onTapTempo=[this]{if(onTapTempo)onTapTempo();};
 namIrBrowser.onImportNam=[this]{if(onImportNam)onImportNam();};namIrBrowser.onImportIr=[this]{if(onImportIr)onImportIr();};namIrBrowser.onLoadSelected=[this]{namIrBrowser.setVisible(false);};
 setSoundState(sound);
}
void LiveScreen::openEditor(std::size_t i){effectEditor.showEffect(i,sound.effects[i]);effectEditor.setBounds(getLocalBounds());effectEditor.setVisible(true);effectEditor.toFront(false);modules[i].setToggleState(true,juce::dontSendNotification);}
void LiveScreen::openLibrary(){namIrBrowser.setBounds(getLocalBounds());namIrBrowser.setVisible(true);namIrBrowser.toFront(false);}
void LiveScreen::setSoundState(const presets::SoundState& s){sound=s;for(std::size_t i=0;i<9;++i){modules[i].getProperties().set("active",sound.effects[i].enabled);modules[i].repaint();}}
void LiveScreen::toggleTuner(){tunerView.setVisible(!tunerView.isVisible());tuner.setToggleState(tunerView.isVisible(),juce::dontSendNotification);if(tunerView.isVisible())tunerView.toFront(false);resized();}
void LiveScreen::setBypassVisual(bool v){bypass.setToggleState(v,juce::dontSendNotification);}
void LiveScreen::setMutedVisual(bool v){mute.setToggleState(v,juce::dontSendNotification);mute.setButtonText(v?"UNMUTE":"MUTE");}
void LiveScreen::setRecordingVisual(bool v){record.setToggleState(v,juce::dontSendNotification);record.setButtonText(v?"STOP & SAVE":"QUICK RECORD");}
void LiveScreen::setStatusText(const std::string& s,bool healthy){status.setText(juce::String(s),juce::dontSendNotification);status.setColour(juce::Label::textColourId,juce::Colour(healthy?Theme::healthy:Theme::mutedText));}
void LiveScreen::setPresetName(const std::string& s){title.setText(juce::String(s).toUpperCase(),juce::dontSendNotification);}
void LiveScreen::setNamName(const std::string& s){namCard.setButtonText(juce::String(s));namIrBrowser.setNamName(s);}
void LiveScreen::setIrName(const std::string& s){irCard.setButtonText(juce::String(s));namIrBrowser.setIrName(s);}
void LiveScreen::setMeters(float in,float out){inputPeak=in;outputPeak=out;repaint();}
void LiveScreen::setTempo(double v){bpm.setValue(v,juce::dontSendNotification);}
void LiveScreen::paint(juce::Graphics& g) {
 g.fillAll(juce::Colour(Theme::background));g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(soundCard.toFloat(),10);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(soundCard.toFloat().reduced(.5f),10,1);
 g.setColour(juce::Colour(Theme::mutedText));g.setFont(11.0f);g.drawText("YOUR SIGNAL CHAIN",0,soundCard.getBottom()+18,200,20,juce::Justification::centredLeft);
 g.drawText("Select an effect to shape your sound",getWidth()-270,soundCard.getBottom()+18,270,20,juce::Justification::centredRight);
 auto meter=[&](int x,float value,const char* label){g.setColour(juce::Colour(Theme::mutedText));g.setFont(9.0f);g.drawText(label,x,2,86,12,juce::Justification::centredLeft);float db=value>0?20*std::log10(value):-60;float norm=juce::jlimit(0.0f,1.0f,(db+60)/60);for(int i=0;i<20;++i){g.setColour(juce::Colour(i<static_cast<int>(norm*20)?(i>17?Theme::danger:Theme::healthy):Theme::border));g.fillRect(x+i*4,20,3,5);}};
 meter(getWidth()-400,inputPeak,"INPUT");meter(getWidth()-300,outputPeak,"OUTPUT");
}
void LiveScreen::resized(){
 const bool compact=getHeight()<620; auto r=getLocalBounds();auto row=r.removeFromTop(compact?32:40);mode.setBounds(row.removeFromLeft(110));mute.setBounds(row.removeFromRight(88).reduced(4,4));bypass.setBounds(row.removeFromRight(90).reduced(4,4));row.removeFromRight(216);status.setBounds(row);
 r.removeFromTop(8);soundCard=r.removeFromTop(compact?104:130);auto s=soundCard.reduced(20,compact?8:14);auto actions=s.removeFromRight(174);chooseSound.setBounds(actions.removeFromTop(36));actions.removeFromTop(10);save.setBounds(actions.removeFromTop(36));
 eyebrow.setBounds(s.removeFromTop(22));title.setBounds(s.removeFromTop(compact?40:48));subtitle.setBounds(s.removeFromTop(compact?26:28));
 r.removeFromTop(32);auto chain=r.removeFromTop(juce::jlimit(110,170,getHeight()/5));int w=(chain.getWidth()-8*10)/9;for(std::size_t i=0;i<9;++i){modules[i].setBounds(chain.removeFromLeft(w));if(i<8)chain.removeFromLeft(10);}
 r.removeFromTop(12);auto assets=r.removeFromTop(compact?68:88);namCard.setBounds(assets.removeFromLeft((assets.getWidth()-12)/2));assets.removeFromLeft(12);irCard.setBounds(assets);
 r.removeFromTop(12);auto footer=r.removeFromTop(compact?32:38);openLooper.setBounds(footer.removeFromRight(132));record.setBounds(footer.removeFromRight(136).reduced(6,0));perform.setBounds(footer.removeFromRight(100).reduced(6,0));tuner.setBounds(footer.removeFromLeft(84));footer.removeFromLeft(10);tempoLabel.setBounds(footer.removeFromLeft(32));bpm.setBounds(footer.removeFromLeft(88));footer.removeFromLeft(8);tap.setBounds(footer.removeFromLeft(108));footer.removeFromLeft(8);click.setBounds(footer.removeFromLeft(100));
 auto level=r.removeFromTop(24);clickLevelLabel.setBounds(level.removeFromLeft(100));clickLevel.setBounds(level.removeFromLeft(140));tunerView.setBounds(getWidth()-350,getHeight()-90,350,82);
 effectEditor.setBounds(getLocalBounds());namIrBrowser.setBounds(getLocalBounds());
}
}

