#pragma once
#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"

namespace pmx::ui
{
class SpecialScreen final : public juce::Component
{
public:
    enum class InstrumentMode { none, synth, bass, piano, violin, drums };

    SpecialScreen();
    void paint(juce::Graphics&) override;
    void resized() override;

    void setStringDrumDetected(int value);
    void setGuitarCalibrationProgress(bool active, int stringIndex, bool completed);
    void setInstrumentMode(InstrumentMode mode);
    [[nodiscard]] InstrumentMode instrumentMode() const noexcept { return selectedInstrument; }

    std::function<void(InstrumentMode)> onInstrumentModeChanged;
    std::function<void(bool)> onRhythmChanged;
    std::function<void(float)> onRhythmLevel;
    std::function<void(int)> onRhythmPattern;
    std::function<void(bool)> onStringDrumsChanged;
    std::function<void(float)> onStringDrumsLevel;
    std::function<void()> onCalibrateGuitar;

private:
    void chooseInstrumentMode(InstrumentMode mode);

    juce::Label eyebrow, title, subtitle;
    juce::Label instrumentHeading, instrumentDetail;
    juce::Label profileHeading, profileDetail;
    juce::Label stringDrumHeading, stringDrumDetail, stringDrumLevelLabel, stringDrumStatus;
    juce::Label beatsHeading, beatsDetail, rhythmLevelLabel;

    std::array<PmxButton, 5> instruments {
        PmxButton{"SYNTH"}, PmxButton{"BASS"}, PmxButton{"PIANO"}, PmxButton{"VIOLIN"}, PmxButton{"DRUMS"}
    };

    PmxButton calibrateGuitar{"LEARN GUITAR", ButtonKind::primary};
    PmxButton stringDrums{"STRING DRUMS OFF"};
    PmxButton rhythm{"BEATS OFF"};
    PmxButton rhythmPattern{"ROCK"};
    juce::Slider stringDrumLevel;
    juce::Slider rhythmLevel;

    juce::Rectangle<int> profileCard, stringDrumCard, beatsCard;
    int rhythmPatternIndex { 1 };
    InstrumentMode selectedInstrument { InstrumentMode::none };
    bool guitarCalibrationActive {};
};
} // namespace pmx::ui
