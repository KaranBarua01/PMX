#include "SettingsScreen.h"
#include "ui/PmxTheme.h"

namespace pmx::ui
{
namespace
{
void styleLabel(juce::Label& label, std::uint32_t colour, float size = 13.0f, bool bold = false)
{
    label.setColour(juce::Label::textColourId, juce::Colour(colour));
    label.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
}
void styleCombo(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(Theme::panelRaised));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colour(Theme::border));
    box.setColour(juce::ComboBox::textColourId, juce::Colour(Theme::text));
    box.setColour(juce::ComboBox::arrowColourId, juce::Colour(Theme::mutedText));
}
}

SettingsScreen::SettingsScreen()
{
    title.setText("Great sound. A reliable connection.", juce::dontSendNotification);
    title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    subtitle.setText("Your Pocket Master handles I/O. PMX handles everything else.", juce::dontSendNotification);
    styleLabel(subtitle, Theme::mutedText);

    connectionTitle.setText("CONNECTION", juce::dontSendNotification); styleLabel(connectionTitle, Theme::mutedText, 11.0f, true);
    connectionState.setText("● POCKET MASTER CONNECTED", juce::dontSendNotification); styleLabel(connectionState, Theme::healthy, 11.0f, true);
    latencyTitle.setText("DRIVER-REPORTED I/O LATENCY", juce::dontSendNotification); styleLabel(latencyTitle, Theme::mutedText, 11.0f, true);
    latencyValue.setText("2.67 ms", juce::dontSendNotification); styleLabel(latencyValue, Theme::healthy, 26.0f, true);
    stabilityTitle.setText("RESPONSE & STABILITY", juce::dontSendNotification); styleLabel(stabilityTitle, Theme::mutedText, 11.0f, true);
    inputGainLabel.setText("INPUT GAIN", juce::dontSendNotification); styleLabel(inputGainLabel, Theme::mutedText, 11.0f, true);
    outputGainLabel.setText("OUTPUT LEVEL", juce::dontSendNotification); styleLabel(outputGainLabel, Theme::mutedText, 11.0f, true);
    helpTitle.setText("Audio interface, not a mystery.", juce::dontSendNotification); styleLabel(helpTitle, Theme::text, 17.0f, true);
    helpText.setText("128 samples is the recommended starting point. Try 64 for lower latency; use 256 if you hear crackles.", juce::dontSendNotification);
    helpText.setMinimumHorizontalScale(0.75f); styleLabel(helpText, Theme::mutedText, 12.0f);

    device.addItem("Pocket Master ASIO",1); device.setSelectedId(1);
    inputChannel.addItem("Input 1",1); inputChannel.setSelectedId(1);
    outputPair.addItem("Output 1 + 2",1); outputPair.setSelectedId(1);
    sampleRate.addItem("48 kHz",1); sampleRate.addItem("44.1 kHz",2); sampleRate.setSelectedId(1);
    bufferSize.addItem("128 samples — Recommended",1); bufferSize.addItem("64 samples — Lowest latency",2); bufferSize.addItem("256 samples — More stable",3); bufferSize.setSelectedId(1);
    for (auto* box : {&device,&inputChannel,&outputPair,&sampleRate,&bufferSize}) { styleCombo(*box); addAndMakeVisible(*box); }

    for (auto* slider : {&inputGain,&outputGain})
    {
        slider->setRange(-24.0, 12.0, 0.1); slider->setValue(0.0);
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,62,24);
        slider->setTextValueSuffix(" dB");
        slider->setColour(juce::Slider::trackColourId,juce::Colour(Theme::accent));
        slider->setColour(juce::Slider::backgroundColourId,juce::Colour(Theme::border));
        addAndMakeVisible(*slider);
    }

    for (auto* c : {static_cast<juce::Component*>(&title),&subtitle,&connectionTitle,&connectionState,&latencyTitle,&latencyValue,
                     &stabilityTitle,&inputGainLabel,&outputGainLabel,&helpTitle,&helpText,&apply,&testAudio,&tryAgain,&showDetails}) addAndMakeVisible(*c);
    tryAgain.setVisible(false); showDetails.setVisible(false);
    apply.onClick=[this]{if(onApply)onApply();};
    testAudio.onClick=[this]{if(onTestAudio)onTestAudio();};
    tryAgain.onClick=[this]{if(onTryAgain)onTryAgain();};
}

void SettingsScreen::setAvailableDevices(const std::vector<audio::AudioDeviceInfo>& devices)
{
    const auto previous=device.getText();
    device.clear(juce::dontSendNotification);
    int id=1, selected=0;
    for(const auto& info:devices)
    {
        device.addItem(juce::String(info.name),id);
        if((!previous.isEmpty()&&previous==info.name)||juce::String(info.name).containsIgnoreCase("Pocket Master")) selected=id;
        ++id;
    }
    if(selected==0 && !devices.empty()) selected=1;
    if(selected>0) device.setSelectedId(selected,juce::dontSendNotification);
}

audio::AudioDeviceSelection SettingsScreen::selectedAudioSetup() const
{
    audio::AudioDeviceSelection out;
    out.deviceName=device.getText().toStdString();
    out.sampleRate=sampleRate.getSelectedId()==2?44100.0:48000.0;
    out.bufferSize=bufferSize.getSelectedId()==2?64:(bufferSize.getSelectedId()==3?256:128);
    out.inputChannel=juce::jmax(0,inputChannel.getSelectedId()-1);
    out.outputLeft=0; out.outputRight=1;
    return out;
}

void SettingsScreen::setConnectionStatus(bool connected,const std::string& connectionText,const std::string& detail,double driverLatencyMs)
{
    connectionState.setText(juce::String(connected?"● ":"● ")+juce::String(connectionText),juce::dontSendNotification);
    connectionState.setColour(juce::Label::textColourId,juce::Colour(connected?Theme::healthy:Theme::danger));
    latencyValue.setText(connected?juce::String(driverLatencyMs,2)+" ms":"—",juce::dontSendNotification);
    latencyValue.setColour(juce::Label::textColourId,juce::Colour(connected?Theme::healthy:Theme::danger));
    helpText.setText(juce::String(detail),juce::dontSendNotification);
    tryAgain.setVisible(!connected); showDetails.setVisible(!connected);
}

void SettingsScreen::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    auto r=getLocalBounds().reduced(24).withTrimmedTop(106);
    auto left=r.removeFromLeft(static_cast<int>(r.getWidth()*0.67f));
    auto right=r;
    const auto drawCard=[&](juce::Rectangle<int> b){g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(b.toFloat(),Theme::cornerRadius);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(b.toFloat(),Theme::cornerRadius,1.0f);};
    drawCard(left.reduced(0,0));
    drawCard(right.reduced(12,0));
}

void SettingsScreen::resized()
{
    auto r=getLocalBounds().reduced(24);
    auto header=r.removeFromTop(86);
    apply.setBounds(header.removeFromRight(150).removeFromTop(38));
    title.setBounds(header.removeFromTop(42)); subtitle.setBounds(header.removeFromTop(28));
    r.removeFromTop(20);

    auto left=r.removeFromLeft(static_cast<int>(r.getWidth()*0.67f)).reduced(18);
    auto right=r.reduced(30,18);
    auto conn=left.removeFromTop(160);
    connectionTitle.setBounds(conn.removeFromTop(22)); connectionState.setBounds(conn.removeFromTop(24));
    auto row=conn.removeFromTop(54); const int w=(row.getWidth()-24)/3;
    device.setBounds(row.removeFromLeft(w)); row.removeFromLeft(12); inputChannel.setBounds(row.removeFromLeft(w)); row.removeFromLeft(12); outputPair.setBounds(row);
    testAudio.setBounds(conn.removeFromTop(38).removeFromLeft(110));
    left.removeFromTop(18);
    stabilityTitle.setBounds(left.removeFromTop(22));
    auto config=left.removeFromTop(50); sampleRate.setBounds(config.removeFromLeft(180)); config.removeFromLeft(12); bufferSize.setBounds(config.removeFromLeft(250));
    left.removeFromTop(12); inputGainLabel.setBounds(left.removeFromTop(20)); inputGain.setBounds(left.removeFromTop(42));
    outputGainLabel.setBounds(left.removeFromTop(20)); outputGain.setBounds(left.removeFromTop(42));

    latencyTitle.setBounds(right.removeFromTop(22)); latencyValue.setBounds(right.removeFromTop(44)); right.removeFromTop(16);
    helpTitle.setBounds(right.removeFromTop(32)); helpText.setBounds(right.removeFromTop(72));
    auto fault=right.removeFromTop(90); tryAgain.setBounds(fault.removeFromBottom(38).removeFromLeft(100)); showDetails.setBounds(fault.removeFromLeft(110));
}
} // namespace pmx::ui
