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
    instrumentDetail.setText("Musical Analysis will drive Synth, Bass, Piano, Violin and Drums. Engines are being built next.", juce::dontSendNotification);

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
        instruments[i].setEnabled(false);
    }

    profileHeading.setText("GUITAR PROFILE", juce::dontSendNotification);
    profileHeading.setColour(juce::Label::textColourId, juce::Colour(Theme::text));
    profileHeading.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    profileDetail.setText("Teach PMX your six open strings. The saved profile supports standard, Drop D and alternate tunings.", juce::dontSendNotification);
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
        stringDrums.setButtonText(stringDrums.getToggleState()?"STRING DRUMS ON":"STRING DRUMS OFF");
        if(onStringDrumsChanged)onStringDrumsChanged(stringDrums.getToggleState());
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
    const auto bounds=getLocalBounds();
    const int top=116;
    const int cardTop=top+72;
    const int cardHeight=112;
    const int lowerTop=cardTop+cardHeight+34;

    g.setColour(juce::Colour(Theme::panel));
    g.fillRoundedRectangle(0.0f,static_cast<float>(lowerTop),static_cast<float>(bounds.getWidth()),116.0f,Theme::cornerRadius);
    g.fillRoundedRectangle(0.0f,static_cast<float>(lowerTop+132),static_cast<float>(bounds.getWidth()),116.0f,Theme::cornerRadius);
    g.fillRoundedRectangle(0.0f,static_cast<float>(lowerTop+264),static_cast<float>(bounds.getWidth()),116.0f,Theme::cornerRadius);
    g.setColour(juce::Colour(Theme::border));
    g.drawRoundedRectangle(0.5f,static_cast<float>(lowerTop)+0.5f,static_cast<float>(bounds.getWidth()-1),115.0f,Theme::cornerRadius,1.0f);
    g.drawRoundedRectangle(0.5f,static_cast<float>(lowerTop+132)+0.5f,static_cast<float>(bounds.getWidth()-1),115.0f,Theme::cornerRadius,1.0f);
    g.drawRoundedRectangle(0.5f,static_cast<float>(lowerTop+264)+0.5f,static_cast<float>(bounds.getWidth()-1),115.0f,Theme::cornerRadius,1.0f);
}

void SpecialScreen::resized()
{
    auto r=getLocalBounds();
    eyebrow.setBounds(r.removeFromTop(22));
    title.setBounds(r.removeFromTop(44));
    subtitle.setBounds(r.removeFromTop(34));
    r.removeFromTop(10);

    instrumentHeading.setBounds(r.removeFromTop(24));
    instrumentDetail.setBounds(r.removeFromTop(28));
    r.removeFromTop(10);

    auto cards=r.removeFromTop(112);
    const int gap=10;
    const int width=(cards.getWidth()-gap*4)/5;
    for(std::size_t i=0;i<instruments.size();++i)
    {
        instruments[i].setBounds(cards.removeFromLeft(width));
        if(i+1<instruments.size())cards.removeFromLeft(gap);
    }

    r.removeFromTop(34);
    auto profile=r.removeFromTop(116).reduced(18,12);
    auto profileAction=profile.removeFromRight(150);
    calibrateGuitar.setBounds(profileAction.withHeight(38).withY(profileAction.getY()+25));
    profileHeading.setBounds(profile.removeFromTop(28));
    profileDetail.setBounds(profile.removeFromTop(44));

    r.removeFromTop(16);
    auto pads=r.removeFromTop(116).reduced(18,12);
    stringDrumHeading.setBounds(pads.removeFromTop(24));
    stringDrumDetail.setBounds(pads.removeFromTop(24));
    auto padControls=pads.removeFromTop(40);
    stringDrums.setBounds(padControls.removeFromLeft(150));
    padControls.removeFromLeft(12);
    stringDrumLevelLabel.setBounds(padControls.removeFromLeft(78));
    stringDrumLevel.setBounds(padControls.removeFromLeft(150));
    padControls.removeFromLeft(14);
    stringDrumStatus.setBounds(padControls);

    r.removeFromTop(16);
    auto beatsBox=r.removeFromTop(116).reduced(18,12);
    beatsHeading.setBounds(beatsBox.removeFromTop(24));
    beatsDetail.setBounds(beatsBox.removeFromTop(24));
    auto beatControls=beatsBox.removeFromTop(40);
    rhythm.setBounds(beatControls.removeFromLeft(110));
    beatControls.removeFromLeft(10);
    rhythmPattern.setBounds(beatControls.removeFromLeft(110));
    beatControls.removeFromLeft(14);
    rhythmLevelLabel.setBounds(beatControls.removeFromLeft(86));
    rhythmLevel.setBounds(beatControls.removeFromLeft(170));
}
} // namespace pmx::ui
