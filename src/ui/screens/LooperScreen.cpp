#include "LooperScreen.h"
#include "ui/PmxTheme.h"
#include <cmath>

namespace pmx::ui
{
LooperScreen::LooperScreen()
{
    title.setText("Make a moment. Build on it.", juce::dontSendNotification);
    title.setFont(juce::FontOptions(30.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    subtitle.setText("ONE LOOP. ONE IDEA. KEEP WHAT FEELS RIGHT.", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    loopName.setText("NEW LOOP", juce::dontSendNotification);
    loopName.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    timing.setText("00:00 / 00:00", juce::dontSendNotification);
    timing.setJustificationType(juce::Justification::centredRight);
    timing.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    loopLevelLabel.setText("LOOP LEVEL", juce::dontSendNotification);
    loopLevelLabel.setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));

    loopLevel.setRange(0.0, 100.0, 1.0);
    loopLevel.setValue(80.0);
    loopLevel.setSliderStyle(juce::Slider::LinearHorizontal);
    loopLevel.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 24);
    loopLevel.setColour(juce::Slider::trackColourId, juce::Colour(Theme::accent));
    loopLevel.setColour(juce::Slider::backgroundColourId, juce::Colour(Theme::border));
    loopLevel.setColour(juce::Slider::textBoxTextColourId, juce::Colour(Theme::text));
    loopLevel.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    juce::Component* components[] = { static_cast<juce::Component*>(&title), &subtitle, &loopName, &timing, &loopLevelLabel, &loopLevel,
                     &saveLoop, &exportWav, &record, &play, &overdub, &stop, &undo, &redo, &clear }; for (auto* c : components) addAndMakeVisible(*c);
    record.onClick=[this]{if(onRecord)onRecord();}; play.onClick=[this]{if(onPlay)onPlay();};
    overdub.onClick=[this]{if(onOverdub)onOverdub();}; stop.onClick=[this]{if(onStop)onStop();};
    undo.onClick=[this]{if(onUndo)onUndo();}; redo.onClick=[this]{if(onRedo)onRedo();};
    clear.onClick=[this]{if(onClear)onClear();}; saveLoop.onClick=[this]{if(onSaveLoop)onSaveLoop();};
    exportWav.onClick=[this]{if(onExportWav)onExportWav();};
    loopLevel.onValueChange=[this]{if(onLoopLevel)onLoopLevel(static_cast<float>(loopLevel.getValue()/100));};
}

void LooperScreen::setAvailability(bool connected,bool hasLoop,bool stopped,bool canUndo,bool canRedo)
{
    record.setEnabled(connected);play.setEnabled(connected&&hasLoop);overdub.setEnabled(connected&&hasLoop);
    undo.setEnabled(connected&&canUndo);redo.setEnabled(connected&&canRedo);
    saveLoop.setEnabled(hasLoop&&stopped);exportWav.setEnabled(hasLoop&&stopped);
}

void LooperScreen::setTiming(std::size_t currentFrames, std::size_t totalFrames, double sampleRate)
{
    const auto format=[](double seconds){const int s=static_cast<int>(seconds+0.5);return juce::String::formatted("%02d:%02d",s/60,s%60);};
    const double current=sampleRate>0.0?static_cast<double>(currentFrames)/sampleRate:0.0;
    const double total=sampleRate>0.0?static_cast<double>(totalFrames)/sampleRate:0.0;
    timing.setText(format(current)+" / "+format(total),juce::dontSendNotification);
}
void LooperScreen::setTransportState(const juce::String& stateText){loopName.setText(stateText,juce::dontSendNotification);}
void LooperScreen::setLoopLevelPercent(float percent){loopLevel.setValue(percent,juce::dontSendNotification);}

void LooperScreen::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    auto wave = getLocalBounds().withTrimmedTop(94).withHeight(220);
    g.setColour(juce::Colour(Theme::panel));
    g.fillRoundedRectangle(wave.toFloat(), static_cast<float>(Theme::cornerRadius));
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(wave.toFloat(), static_cast<float>(Theme::cornerRadius), 1.0f);

    auto graph = wave.reduced(18, 48);
    const int bars = static_cast<int>(peaks.size());
    const float step = static_cast<float>(graph.getWidth()) / static_cast<float>(bars);
    for (int i = 0; i < bars; ++i)
    {
        const float h = juce::jmax(2.0f,juce::jlimit(0.0f,1.0f,peaks[static_cast<std::size_t>(i)])*static_cast<float>(graph.getHeight()));
        const float x = static_cast<float>(graph.getX()) + i * step;
        const bool played = static_cast<float>(i)/bars < progress;
        g.setColour(juce::Colour(played ? Theme::accent : Theme::border).withAlpha(played ? 0.95f : 0.9f));
        g.fillRoundedRectangle(x, static_cast<float>(graph.getCentreY()) - h * 0.5f,
                               juce::jmax(2.0f, step * 0.5f), h, 1.5f);
    }
}

void LooperScreen::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop(94);
    auto actions = header.removeFromRight(250).removeFromTop(38);
    exportWav.setBounds(actions.removeFromRight(110));
    saveLoop.setBounds(actions.removeFromRight(120).reduced(4,0));
    title.setBounds(header.removeFromTop(42));
    subtitle.setBounds(header.removeFromTop(26));

    auto wave = r.removeFromTop(220);
    loopName.setBounds(wave.getX()+18, wave.getY()+12, 240, 28);
    timing.setBounds(wave.getRight()-180, wave.getBottom()-38, 160, 26);
    r.removeFromTop(16);

    auto transport = r.removeFromTop(98);
    const int gap = 10;
    const int w = (transport.getWidth() - gap*3) / 4;
    record.setBounds(transport.removeFromLeft(w)); transport.removeFromLeft(gap);
    play.setBounds(transport.removeFromLeft(w)); transport.removeFromLeft(gap);
    overdub.setBounds(transport.removeFromLeft(w)); transport.removeFromLeft(gap);
    stop.setBounds(transport);

    r.removeFromTop(14);
    auto lower = r.removeFromTop(42);
    undo.setBounds(lower.removeFromLeft(86)); lower.removeFromLeft(8);
    redo.setBounds(lower.removeFromLeft(86)); lower.removeFromLeft(8);
    clear.setBounds(lower.removeFromLeft(92));
    auto level = lower.removeFromRight(330);
    loopLevelLabel.setBounds(level.removeFromLeft(90));
    loopLevel.setBounds(level);
}
} // namespace pmx::ui
