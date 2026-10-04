#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/components/PmxButton.h"

namespace pmx::ui
{
class LooperScreen final : public juce::Component
{
public:
    LooperScreen();
    void paint(juce::Graphics&) override;
    void resized() override;
    void setTiming(std::size_t currentFrames, std::size_t totalFrames, double sampleRate);
    void setTransportState(const juce::String& stateText);
    void setLoopLevelPercent(float percent);
    void setWaveform(const std::array<float,128>& values, float fraction) { peaks=values;progress=fraction;repaint(); }
    void setAvailability(bool connected,bool hasLoop,bool stopped,bool canUndo,bool canRedo);
    std::function<void(float)> onLoopLevel;

    std::function<void()> onRecord;
    std::function<void()> onPlay;
    std::function<void()> onOverdub;
    std::function<void()> onStop;
    std::function<void()> onUndo;
    std::function<void()> onRedo;
    std::function<void()> onClear;
    std::function<void()> onSaveLoop;
    std::function<void()> onExportWav;

private:
    juce::Label title;
    juce::Label subtitle;
    juce::Label loopName;
    juce::Label timing;
    juce::Label loopLevelLabel;
    juce::Slider loopLevel;
    PmxButton saveLoop { "SAVE LOOP", ButtonKind::primary };
    PmxButton exportWav { "EXPORT WAV" };
    PmxButton record { "RECORD" };
    PmxButton play { "PLAY", ButtonKind::primary };
    PmxButton overdub { "OVERDUB" };
    PmxButton stop { "STOP" };
    PmxButton undo { "UNDO" };
    PmxButton redo { "REDO" };
    PmxButton clear { "CLEAR", ButtonKind::danger };
    std::array<float,128> peaks{};
    float progress{};
};
} // namespace pmx::ui
