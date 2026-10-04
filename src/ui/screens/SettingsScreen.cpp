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
    connectionState.setText("POCKET MASTER OFFLINE", juce::dontSendNotification); styleLabel(connectionState, Theme::healthy, 11.0f, true);
    latencyTitle.setText("DRIVER-REPORTED I/O LATENCY", juce::dontSendNotification); styleLabel(latencyTitle, Theme::mutedText, 11.0f, true);
    latencyValue.setText("—", juce::dontSendNotification); styleLabel(latencyValue, Theme::healthy, 26.0f, true);
    stabilityTitle.setText("RESPONSE & STABILITY", juce::dontSendNotification); styleLabel(stabilityTitle, Theme::mutedText, 11.0f, true);
    inputGainLabel.setText("INPUT GAIN", juce::dontSendNotification); styleLabel(inputGainLabel, Theme::mutedText, 11.0f, true);
    outputGainLabel.setText("OUTPUT LEVEL", juce::dontSendNotification); styleLabel(outputGainLabel, Theme::mutedText, 11.0f, true);
    helpTitle.setText("Audio interface, not a mystery.", juce::dontSendNotification); styleLabel(helpTitle, Theme::text, 17.0f, true);
    helpText.setText("128 samples is the recommended starting point. Try 64 for lower latency; use 256 if you hear crackles.", juce::dontSendNotification);
    helpText.setMinimumHorizontalScale(0.75f); styleLabel(helpText, Theme::mutedText, 12.0f);

    device.setTextWhenNothingSelected("No ASIO device");
    inputChannel.setTextWhenNothingSelected("No input");outputPair.setTextWhenNothingSelected("No output");
    sampleRate.setTextWhenNothingSelected("Unavailable");bufferSize.setTextWhenNothingSelected("Unavailable");
    device.onChange=[this]{refreshChannels();};
    for (auto* box : {&device,&inputChannel,&outputPair,&sampleRate,&bufferSize}) { styleCombo(*box); addAndMakeVisible(*box); }

    for (auto* slider : {&inputGain,&outputGain})
    {
        slider->setRange(-24.0, slider==&outputGain?6.0:24.0, 0.1); slider->setValue(0.0);
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,62,24);
        slider->setTextValueSuffix(" dB");
        slider->setColour(juce::Slider::trackColourId,juce::Colour(Theme::accent));
        slider->setColour(juce::Slider::backgroundColourId,juce::Colour(Theme::border));
        addAndMakeVisible(*slider);
    }

    juce::Component* components[] = {static_cast<juce::Component*>(&title),&subtitle,&connectionTitle,&connectionState,&latencyTitle,&latencyValue,
                     &stabilityTitle,&inputGainLabel,&outputGainLabel,&helpTitle,&helpText,&apply,&testAudio,&tryAgain,&showDetails}; for (auto* c : components) addAndMakeVisible(*c);
    tryAgain.setVisible(true); showDetails.setVisible(true);
    showDetails.onClick=[this]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Audio details",juce::String(detailText));};
    apply.onClick=[this]{if(onApply)onApply();};
    testAudio.onClick=[this]{if(onTestAudio)onTestAudio();};
    tryAgain.onClick=[this]{if(onTryAgain)onTryAgain();};
}

void SettingsScreen::setAvailableDevices(const std::vector<audio::AudioDeviceInfo>& devices)
{
    available=devices;const auto previous=device.getText();device.clear(juce::dontSendNotification);
    int id=1,selected=0;
    for(const auto& info:available){if(!info.asio)continue;device.addItem(juce::String(info.name),id);if(previous==juce::String(info.name))selected=id;if(selected==0&&juce::String(info.name).containsIgnoreCase("Pocket Master"))selected=id;++id;}
    if(selected>0)device.setSelectedId(selected,juce::dontSendNotification);
    refreshChannels();
}
void SettingsScreen::refreshChannels()
{
    const auto oldRate=sampleRate.getText(),oldBuffer=bufferSize.getText();
    inputChannel.clear(juce::dontSendNotification);outputPair.clear(juce::dontSendNotification);sampleRate.clear(juce::dontSendNotification);bufferSize.clear(juce::dontSendNotification);rates.clear();buffers.clear();
    const auto selected=device.getText().toStdString();
    const auto it=std::find_if(available.begin(),available.end(),[&](const auto& d){return d.name==selected&&d.asio;});
    if(it==available.end()){apply.setEnabled(false);return;}
    apply.setEnabled(true);
    for(int i=0;i<it->inputChannels;++i)inputChannel.addItem("Channel "+juce::String(i+1)+" · Guitar",i+1);
    for(int i=0;i+1<it->outputChannels;i+=2)outputPair.addItem("Channels "+juce::String(i+1)+" + "+juce::String(i+2),i/2+1);
    rates=it->sampleRates;buffers=it->bufferSizes;
    for(std::size_t i=0;i<rates.size();++i)sampleRate.addItem(juce::String(rates[i]/1000,1)+" kHz",static_cast<int>(i)+1);
    for(std::size_t i=0;i<buffers.size();++i){auto text=juce::String(buffers[i])+" samples";if(buffers[i]==128)text+=" · Recommended";else if(buffers[i]==64)text+=" · Lowest latency";else if(buffers[i]==256)text+=" · More stable";bufferSize.addItem(text,static_cast<int>(i)+1);}
    inputChannel.setSelectedId(1,juce::dontSendNotification);outputPair.setSelectedId(1,juce::dontSendNotification);
    for(std::size_t i=0;i<rates.size();++i)if(rates[i]==44100||sampleRate.getItemText(static_cast<int>(i))==oldRate)sampleRate.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);
    for(std::size_t i=0;i<buffers.size();++i)if(buffers[i]==128||bufferSize.getItemText(static_cast<int>(i))==oldBuffer)bufferSize.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);
    if(sampleRate.getSelectedId()==0&& !rates.empty())sampleRate.setSelectedId(1,juce::dontSendNotification);
    if(bufferSize.getSelectedId()==0&& !buffers.empty())bufferSize.setSelectedId(1,juce::dontSendNotification);
}
void SettingsScreen::setSelection(const audio::AudioDeviceSelection& selection)
{
    device.setText(juce::String(selection.deviceName),juce::dontSendNotification);refreshChannels();
    inputChannel.setSelectedId(selection.inputChannel+1,juce::dontSendNotification);outputPair.setSelectedId(selection.outputLeft/2+1,juce::dontSendNotification);
    for(std::size_t i=0;i<rates.size();++i)if(rates[i]==selection.sampleRate)sampleRate.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);
    for(std::size_t i=0;i<buffers.size();++i)if(buffers[i]==selection.bufferSize)bufferSize.setSelectedId(static_cast<int>(i)+1,juce::dontSendNotification);
}
audio::AudioDeviceSelection SettingsScreen::selectedAudioSetup() const
{
    audio::AudioDeviceSelection out;out.deviceName=device.getText().toStdString();
    const int r=sampleRate.getSelectedId()-1,b=bufferSize.getSelectedId()-1;
    out.sampleRate=r>=0&&static_cast<std::size_t>(r)<rates.size()?rates[static_cast<std::size_t>(r)]:0;
    out.bufferSize=b>=0&&static_cast<std::size_t>(b)<buffers.size()?buffers[static_cast<std::size_t>(b)]:0;
    out.inputChannel=juce::jmax(0,inputChannel.getSelectedId()-1);out.outputLeft=juce::jmax(0,outputPair.getSelectedId()-1)*2;out.outputRight=out.outputLeft+1;return out;
}
void SettingsScreen::setConnectionStatus(bool connected,const std::string& connectionText,const std::string& detail,double driverLatencyMs)
{
    detailText=detail;
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
    for(auto card:{connectionCard,stabilityCard,gainCard,latencyCard,helpCard}){g.setColour(juce::Colour(Theme::panel));g.fillRoundedRectangle(card.toFloat(),10);g.setColour(juce::Colour(Theme::border));g.drawRoundedRectangle(card.toFloat().reduced(.5f),10,1);}
}
void SettingsScreen::resized()
{
    auto r=getLocalBounds();auto header=r.removeFromTop(82);apply.setBounds(header.removeFromRight(150).removeFromTop(36));title.setBounds(header.removeFromTop(42));subtitle.setBounds(header.removeFromTop(26));
    auto left=r.removeFromLeft(static_cast<int>(r.getWidth()*.63));r.removeFromLeft(18);auto right=r;
    connectionCard=left.removeFromTop(160);left.removeFromTop(14);stabilityCard=left.removeFromTop(112);left.removeFromTop(14);gainCard=left.removeFromTop(144);
    latencyCard=right.removeFromTop(156);right.removeFromTop(14);helpCard=right.removeFromTop(260);
    auto conn=connectionCard.reduced(18);connectionTitle.setBounds(conn.removeFromTop(20));connectionState.setBounds(conn.removeFromTop(24));auto row=conn.removeFromTop(36);int w=(row.getWidth()-16)/3;device.setBounds(row.removeFromLeft(w));row.removeFromLeft(8);inputChannel.setBounds(row.removeFromLeft(w));row.removeFromLeft(8);outputPair.setBounds(row);conn.removeFromTop(10);testAudio.setBounds(conn.removeFromTop(30).removeFromLeft(120));
    auto stable=stabilityCard.reduced(18);stabilityTitle.setBounds(stable.removeFromTop(24));auto config=stable.removeFromTop(38);sampleRate.setBounds(config.removeFromLeft(130));config.removeFromLeft(10);bufferSize.setBounds(config);
    auto gain=gainCard.reduced(18);auto in=gain.removeFromLeft((gain.getWidth()-18)/2);gain.removeFromLeft(18);auto out=gain;inputGainLabel.setBounds(in.removeFromTop(24));inputGain.setBounds(in.removeFromTop(42));outputGainLabel.setBounds(out.removeFromTop(24));outputGain.setBounds(out.removeFromTop(42));
    auto latency=latencyCard.reduced(18);latencyTitle.setBounds(latency.removeFromTop(26));latencyValue.setBounds(latency.removeFromTop(50));
    auto help=helpCard.reduced(18);helpTitle.setBounds(help.removeFromTop(36));helpText.setBounds(help.removeFromTop(110));auto fault=help.removeFromBottom(34);tryAgain.setBounds(fault.removeFromLeft(100));fault.removeFromLeft(8);showDetails.setBounds(fault.removeFromLeft(116));
}
} // namespace pmx::ui
