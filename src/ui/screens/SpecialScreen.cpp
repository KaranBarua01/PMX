#include "SpecialScreen.h"
#include "ui/PmxTheme.h"

namespace pmx::ui
{
SpecialScreen::SpecialScreen()
{
    for (auto* label : {&eyebrow,&title,&subtitle,&instrumentHeading,&instrumentDetail,&profileHeading,&profileDetail,
                        &stringDrumHeading,&stringDrumDetail,&stringDrumLevelLabel,&stringDrumStatus,
                        &beatsHeading,&beatsDetail,&rhythmLevelLabel})
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, juce::Colour(Theme::mutedText));
    }

    eyebrow.setText("PMX LAB", juce::dontSendNotification);
    eyebrow.setColour(juce::Label::textColourId, juce::Colour(Theme::accent));
    eyebrow.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    title.setText("SPECIAL", juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    title.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    subtitle.setText("Experimental performance tools live here so the main guitar screen stays simple.", juce::dontSendNotification);

    instrumentHeading.setText("GUITAR -> INSTRUMENTS", juce::dontSendNotification);
    instrumentHeading.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    instrumentHeading.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    instrumentDetail.setText("Choose one guitar-triggered instrument. Selecting another switches modes without stacking sounds.", juce::dontSendNotification);

    static constexpr const char* subtitles[] {
        "Pitch, bend, slide and vibrato",
        "Monophonic note tracking + octave rules",
        "Notes, chords, velocity and duration",
        "Pitch movement + optional expression",
        "Detected note or gesture to percussion"
    };
    for (std::size_t i=0;i<instruments.size();++i)
    {
        addAndMakeVisible(instruments[i]);
        instruments[i].getProperties().set("category","INSTRUMENT ENGINE");
        instruments[i].getProperties().set("subtitle",subtitles[i]);
        instruments[i].getProperties().set("active",false);
        instruments[i].setEnabled(true);
        instruments[i].onClick=[this,i]{
            const auto mode=static_cast<InstrumentMode>(static_cast<int>(i)+1);
            chooseInstrumentMode(selectedInstrument==mode?InstrumentMode::none:mode);
        };
    }

    profileHeading.setText("GUITAR PROFILE", juce::dontSendNotification);
    profileHeading.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    profileHeading.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    profileDetail.setText("Teach PMX your six open strings. The saved profile supports your learned six-string tuning.", juce::dontSendNotification);
    addAndMakeVisible(calibrateGuitar);
    calibrateGuitar.onClick=[this]{if(onCalibrateGuitar)onCalibrateGuitar();};

    stringDrumHeading.setText("STRING DRUMS", juce::dontSendNotification);
    stringDrumHeading.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    stringDrumHeading.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    stringDrumDetail.setText("Experimental open-string percussion trigger.", juce::dontSendNotification);
    stringDrumLevelLabel.setText("PAD LEVEL", juce::dontSendNotification);
    stringDrumStatus.setText("E=KICK  A=SNARE  D=HAT  G=OPEN HAT  B=TOM  HIGH E=CRASH", juce::dontSendNotification);
    addAndMakeVisible(stringDrums);
    addAndMakeVisible(stringDrumLevel);
    stringDrums.setClickingTogglesState(true);
    stringDrums.onClick=[this]{
        const bool enabled=stringDrums.getToggleState();
        setInstrumentMode(enabled?InstrumentMode::drums:InstrumentMode::none);
        if(onStringDrumsChanged)onStringDrumsChanged(enabled);
    };
    stringDrumLevel.setRange(0,100,1);
    stringDrumLevel.setValue(55);
    stringDrumLevel.setSliderStyle(juce::Slider::LinearHorizontal);
    stringDrumLevel.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    stringDrumLevel.onValueChange=[this]{if(onStringDrumsLevel)onStringDrumsLevel(static_cast<float>(stringDrumLevel.getValue()/100.0));};

    beatsHeading.setText("BEATS", juce::dontSendNotification);
    beatsHeading.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    beatsHeading.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    beatsDetail.setText("BPM-synced backing drums for practice, looping and quick recording.", juce::dontSendNotification);
    rhythmLevelLabel.setText("BEAT LEVEL", juce::dontSendNotification);
    addAndMakeVisible(rhythm);
    addAndMakeVisible(rhythmPattern);
    addAndMakeVisible(rhythmLevel);
    rhythm.setClickingTogglesState(true);
    rhythm.onClick=[this]{
        rhythm.setButtonText(rhythm.getToggleState()?"BEATS ON":"BEATS OFF");
        if(onRhythmChanged)onRhythmChanged(rhythm.getToggleState());
    };
    rhythmPattern.onClick=[this]{
        static constexpr const char* names[]{"STRAIGHT","ROCK","POP","DRIVE"};
        rhythmPatternIndex=(rhythmPatternIndex+1)%4;
        rhythmPattern.setButtonText(names[rhythmPatternIndex]);
        if(onRhythmPattern)onRhythmPattern(rhythmPatternIndex);
    };
    rhythmLevel.setRange(0,100,1);
    rhythmLevel.setValue(35);
    rhythmLevel.setSliderStyle(juce::Slider::LinearHorizontal);
    rhythmLevel.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    rhythmLevel.onValueChange=[this]{if(onRhythmLevel)onRhythmLevel(static_cast<float>(rhythmLevel.getValue()/100.0));};
}

void SpecialScreen::chooseInstrumentMode(InstrumentMode mode)
{
    setInstrumentMode(mode);
    if(onInstrumentModeChanged)onInstrumentModeChanged(mode);
}

void SpecialScreen::setInstrumentMode(InstrumentMode mode)
{
    selectedInstrument=mode;
    for(std::size_t i=0;i<instruments.size();++i)
    {
        const auto buttonMode=static_cast<InstrumentMode>(static_cast<int>(i)+1);
        const bool active=buttonMode==mode;
        instruments[i].setToggleState(active,juce::dontSendNotification);
        instruments[i].getProperties().set("active",active);
        instruments[i].repaint();
    }

    const bool drumsActive=mode==InstrumentMode::drums;
    stringDrums.setToggleState(drumsActive,juce::dontSendNotification);
    stringDrums.setButtonText(drumsActive?"STRING DRUMS ON":"STRING DRUMS OFF");
}

void SpecialScreen::setStringDrumDetected(int value)
{
    if(guitarCalibrationActive)return;
    static constexpr const char* names[]{"LOW E -> KICK","A -> SNARE","D -> CLOSED HAT","G -> OPEN HAT","B -> TOM","HIGH E -> CRASH"};
    if(value>=0&&value<6)
        stringDrumStatus.setText(juce::String("LAST HIT: ")+names[value],juce::dontSendNotification);
    else
        stringDrumStatus.setText("E=KICK  A=SNARE  D=HAT  G=OPEN HAT  B=TOM  HIGH E=CRASH",juce::dontSendNotification);
}

void SpecialScreen::setGuitarCalibrationProgress(bool active,int stringIndex,bool completed)
{
    guitarCalibrationActive=active;
    calibrateGuitar.setButtonText(active?"CANCEL LEARN":"LEARN GUITAR");
    static constexpr const char* names[]{"LOW E","A","D","G","B","HIGH E"};
    if(active&&stringIndex>=0&&stringIndex<6)
        stringDrumStatus.setText(juce::String("LEARN GUITAR: PLUCK ")+names[stringIndex]+" AND LET IT RING",juce::dontSendNotification);
    else if(completed)
        stringDrumStatus.setText("GUITAR PROFILE SAVED",juce::dontSendNotification);
    else
        setStringDrumDetected(-1);
}

void SpecialScreen::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(Theme::background));
    for(const auto card : {profileCard,stringDrumCard,beatsCard})
    {
        if(card.isEmpty())continue;
        g.setColour(juce::Colour(Theme::panel));
        g.fillRoundedRectangle(card.toFloat(),Theme::cornerRadius);
        g.setColour(juce::Colour(Theme::border));
        g.drawRoundedRectangle(card.toFloat().reduced(0.5f),Theme::cornerRadius,1.0f);
    }
}

void SpecialScreen::resized()
{
    auto r=getLocalBounds();

    eyebrow.setBounds(r.removeFromTop(18));
    title.setBounds(r.removeFromTop(36));
    subtitle.setBounds(r.removeFromTop(28));
    r.removeFromTop(4);

    instrumentHeading.setBounds(r.removeFromTop(22));
    instrumentDetail.setBounds(r.removeFromTop(24));
    r.removeFromTop(6);

    auto cards=r.removeFromTop(96);
    const int gap=10;
    const int width=(cards.getWidth()-gap*4)/5;
    for(std::size_t i=0;i<instruments.size();++i)
    {
        instruments[i].setBounds(cards.removeFromLeft(width));
        if(i+1<instruments.size())cards.removeFromLeft(gap);
    }

    r.removeFromTop(14);
    profileCard=r.removeFromTop(80);
    {
        auto box=profileCard.reduced(16,8);
        auto action=box.removeFromRight(150);
        calibrateGuitar.setBounds(action.withHeight(36).withY(action.getY()+14));
        profileHeading.setBounds(box.removeFromTop(26));
        profileDetail.setBounds(box);
    }

    r.removeFromTop(10);
    stringDrumCard=r.removeFromTop(90);
    {
        auto box=stringDrumCard.reduced(16,8);
        stringDrumHeading.setBounds(box.removeFromTop(22));
        stringDrumDetail.setBounds(box.removeFromTop(20));
        auto controls=box.removeFromTop(32);
        stringDrums.setBounds(controls.removeFromLeft(150));
        controls.removeFromLeft(10);
        stringDrumLevelLabel.setBounds(controls.removeFromLeft(76));
        stringDrumLevel.setBounds(controls.removeFromLeft(140));
        controls.removeFromLeft(12);
        stringDrumStatus.setBounds(controls);
    }

    r.removeFromTop(10);
    beatsCard=r.removeFromTop(90);
    {
        auto box=beatsCard.reduced(16,8);
        beatsHeading.setBounds(box.removeFromTop(22));
        beatsDetail.setBounds(box.removeFromTop(20));
        auto controls=box.removeFromTop(32);
        rhythm.setBounds(controls.removeFromLeft(110));
        controls.removeFromLeft(10);
        rhythmPattern.setBounds(controls.removeFromLeft(110));
        controls.removeFromLeft(12);
        rhythmLevelLabel.setBounds(controls.removeFromLeft(84));
        rhythmLevel.setBounds(controls.removeFromLeft(160));
    }
}
} // namespace pmx::ui
