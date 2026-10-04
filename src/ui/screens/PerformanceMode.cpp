#include "PerformanceMode.h"
#include "ui/PmxTheme.h"

namespace pmx::ui
{
PerformanceMode::PerformanceMode()
{
    title.setText("Your sound. Your space.", juce::dontSendNotification);
    title.setFont(juce::FontOptions(27.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    subtitle.setText("Only the controls you need while your hands are on the guitar.", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    preset.setText("DREAM CLEAN", juce::dontSendNotification); preset.setFont(juce::FontOptions(30.0f,juce::Font::bold)); preset.setColour(juce::Label::textColourId,juce::Colour(Theme::text));
    connection.setText("POCKET MASTER OFFLINE",juce::dontSendNotification);connection.setColour(juce::Label::textColourId,juce::Colour(Theme::healthy));connection.setJustificationType(juce::Justification::centredRight);
    shortcutHint.setText("B bypass   M mute   T tuner   Space loop   S stop   P tap   R record",juce::dontSendNotification);shortcutHint.setColour(juce::Label::textColourId,juce::Colour(Theme::mutedText));
    delayTime.setRange(1,2000,1);delayTime.setValue(420);feedback.setRange(0,95,1);feedback.setValue(23);mix.setRange(0,100,1);mix.setValue(24);
    inputMeter.setLevel(0); outputMeter.setLevel(0);
    juce::Component* components[] = {static_cast<juce::Component*>(&title),&subtitle,&preset,&connection,&shortcutHint,&exit,&tuner,&bypass,&mute,&loop,&quickRecord,&tap,&delayTime,&feedback,&mix,&inputMeter,&outputMeter}; for (auto* c : components) addAndMakeVisible(*c);
    for(auto& b:chain)addAndMakeVisible(b);
    tuner.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::tuner);};
    bypass.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::bypass);};
    mute.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::mute);};
    loop.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::loopTransport);};
    quickRecord.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::quickRecord);};
    tap.onClick=[this]{if(onShortcut)onShortcut(input::ShortcutCommand::tapTempo);};
    constexpr std::array<std::size_t,6> indexes{1,2,3,4,7,8};
    for(std::size_t i=0;i<chain.size();++i){const auto effect=indexes[i];chain[i].onClick=[this,effect]{if(onEditEffect)onEditEffect(effect);};}
    auto changed=[this]{if(onDelayChanged)onDelayChanged(static_cast<float>(delayTime.getValue()),static_cast<float>(feedback.getValue()),static_cast<float>(mix.getValue()));};
    delayTime.onValueChange=feedback.onValueChange=mix.onValueChange=changed;
    exit.onClick=[this]{if(onExit)onExit();};
}

void PerformanceMode::setState(const std::string& name,const std::string& status,bool isConnected,bool isMuted,bool isBypassed,float in,float out,double bpm)
{
    preset.setText(juce::String(name).toUpperCase(),juce::dontSendNotification);connection.setText(juce::String(status),juce::dontSendNotification);
    connection.setColour(juce::Label::textColourId,juce::Colour(isConnected?Theme::healthy:Theme::mutedText));
    mute.setButtonText(isMuted?"UNMUTE":"MUTE");bypass.setToggleState(isBypassed,juce::dontSendNotification);
    inputMeter.setLevel(in);outputMeter.setLevel(out);tap.setButtonText("TAP "+juce::String(bpm,0)+" BPM");
}
void PerformanceMode::setDelay(float t,float f,float m){delayTime.setValue(t,juce::dontSendNotification);feedback.setValue(f,juce::dontSendNotification);mix.setValue(m,juce::dontSendNotification);}
void PerformanceMode::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    auto area=getLocalBounds().withTrimmedTop(100);
    const int gap=12;
    const int col=(area.getWidth()-gap*2)/3;
    auto left=area.removeFromLeft(col);area.removeFromLeft(gap);auto centre=area.removeFromLeft(col);area.removeFromLeft(gap);auto right=area;
    const auto card=[&](juce::Rectangle<int> r){g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(r.toFloat(),Theme::cornerRadius);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(r.toFloat(),Theme::cornerRadius,1.0f);};
    card(left.withHeight(208)); card(left.withTrimmedTop(222).withHeight(208));
    card(centre.withHeight(208)); card(centre.withTrimmedTop(222).withHeight(208));
    card(right.withHeight(208)); card(right.withTrimmedTop(222).withHeight(208));
}

void PerformanceMode::resized()
{
    auto r=getLocalBounds(); auto head=r.removeFromTop(90); exit.setBounds(head.removeFromRight(150).removeFromTop(38)); connection.setBounds(head.removeFromRight(330).removeFromTop(30)); title.setBounds(head.removeFromTop(40)); subtitle.setBounds(head.removeFromTop(28));
    const int gap=12; const int col=(r.getWidth()-gap*2)/3; auto left=r.removeFromLeft(col);r.removeFromLeft(gap);auto centre=r.removeFromLeft(col);r.removeFromLeft(gap);auto right=r;
    auto live=left.withHeight(208).reduced(16);preset.setBounds(live.removeFromTop(42));tuner.setBounds(live.removeFromTop(36));live.removeFromTop(8);bypass.setBounds(live.removeFromTop(36));live.removeFromTop(8);mute.setBounds(live.removeFromTop(36));
    auto chainArea=left.withTrimmedTop(222).withHeight(208).reduced(14);int cw=chainArea.getWidth()/3;for(std::size_t i=0;i<chain.size();++i){int row=static_cast<int>(i)/3,colIndex=static_cast<int>(i)%3;chain[i].setBounds(chainArea.getX()+colIndex*cw,chainArea.getY()+row*72,cw-8,58);}
    auto delay=centre.withHeight(208).reduced(12);int kw=delay.getWidth()/3;delayTime.setBounds(delay.removeFromLeft(kw));feedback.setBounds(delay.removeFromLeft(kw));mix.setBounds(delay);
    auto meters=centre.withTrimmedTop(222).withHeight(208).reduced(18);inputMeter.setBounds(meters.removeFromTop(24));meters.removeFromTop(18);outputMeter.setBounds(meters.removeFromTop(24));shortcutHint.setBounds(meters.removeFromBottom(44));
    auto transport=right.withHeight(208).reduced(16);loop.setBounds(transport.removeFromTop(42));transport.removeFromTop(10);quickRecord.setBounds(transport.removeFromTop(42));transport.removeFromTop(10);tap.setBounds(transport.removeFromTop(42));
    auto presetCard=right.withTrimmedTop(222).withHeight(208).reduced(18);juce::ignoreUnused(presetCard);
}
} // namespace pmx::ui
