#pragma once
#include <atomic>
#include <array>
#include <vector>
#include "AudioSink.h"
#include "SignalMetrics.h"
#include "TempoService.h"
#include "analysis/TunerTap.h"
#include "analysis/MusicalAnalysisEngine.h"
#include "analysis/FretboardNoteSolver.h"
#include "analysis/PolyphonicNoteDetector.h"
#include "performance/PerformanceEventEngine.h"
#include "instruments/BassInstrumentEngine.h"
#include "instruments/SynthInstrumentEngine.h"
#include "instruments/PianoInstrumentEngine.h"
#include "instruments/ViolinInstrumentEngine.h"
#include "dsp/OutputProtector.h"
#include "dsp/GuitarRack.h"
#include "dsp/Metronome.h"
#include "dsp/RhythmDrumMachine.h"
#include "dsp/GuitarDrumTrigger.h"
#include "ir/IrProcessor.h"
#include "nam/NamProcessor.h"
#include "looper/LooperEngine.h"
#include "recording/QuickRecorder.h"

namespace pmx::audio
{
enum class LoopCommand { record, play, overdub, stop, undo, redo, clear };
struct LoopStatus
{
    looper::LooperState state{looper::LooperState::empty};
    std::size_t recordedFrames{}, loopFrames{}, position{};
    bool canUndo{}, canRedo{}, pendingCommands{};
};
class ProcessingEngine final : public AudioSink
{
public:
    ProcessingEngine();
    void prepare(double sampleRate, int maxBlockSize, int inputChannels, int outputChannels) override;
    void process(const float* const* inputs, int numInputs,
                 float* const* outputs, int numOutputs,
                 int numSamples) noexcept override;
    void stopped() noexcept override;

    void setFxBypass(bool shouldBypass) noexcept { fxBypass.store(shouldBypass, std::memory_order_relaxed); }
    void setMuted(bool shouldMute) noexcept { muted.store(shouldMute, std::memory_order_relaxed); }
    [[nodiscard]] bool isFxBypassed() const noexcept { return fxBypass.load(std::memory_order_relaxed); }
    [[nodiscard]] bool isMuted() const noexcept { return muted.load(std::memory_order_relaxed); }
    [[nodiscard]] SignalMetricsSnapshot metrics() const noexcept { return signalMetrics.snapshot(); }
    [[nodiscard]] double sampleRate() const noexcept { return preparedSampleRate; }
    [[nodiscard]] int maxBlockSize() const noexcept { return preparedBlockSize; }

    dsp::GuitarRack& rack() noexcept { return guitarRack; }
    const dsp::GuitarRack& rack() const noexcept { return guitarRack; }
    nam::NamProcessor& nam() noexcept { return namProcessor; }
    ir::IrProcessor& ir() noexcept { return irProcessor; }
    TempoService& tempo() noexcept { return tempoService; }
    analysis::TunerTap& tunerTap() noexcept { return tunerCapture; }
    [[nodiscard]] analysis::MusicalAnalysisSnapshot musicalAnalysis() const noexcept { return musicalAnalyzer.snapshot(); }
    [[nodiscard]] analysis::ResolvedNoteSnapshot resolvedNote() const noexcept { return noteSolver.snapshot(); }
    [[nodiscard]] analysis::PolyphonicSnapshot polyphonicNotes() const noexcept { return polyphonicDetector.snapshot(); }
    [[nodiscard]] bool popPerformanceEvent(performance::PerformanceEvent& event) noexcept { return performanceEvents.pop(event); }
    [[nodiscard]] std::uint64_t droppedPerformanceEvents() const noexcept { return performanceEvents.droppedCount(); }
    void setGuitarProfile(const instruments::GuitarProfile& profile) noexcept { noteSolver.setProfile(profile); polyphonicDetector.setProfile(profile); }
    looper::LooperEngine& looper() noexcept { return looperEngine; }
    const looper::LooperEngine& looper() const noexcept { return looperEngine; }
    recording::QuickRecorder& recorder() noexcept { return quickRecorder; }
    const recording::QuickRecorder& recorder() const noexcept { return quickRecorder; }
    void setMetronomeEnabled(bool enabled) noexcept { metronome.setEnabled(enabled); }
    void setMetronomeLevel(float level) noexcept { metronome.setLevel(level); }
    void setRhythmEnabled(bool enabled) noexcept { rhythmDrums.setEnabled(enabled); }
    void setRhythmLevel(float level) noexcept { rhythmDrums.setLevel(level); }
    void setRhythmPattern(int pattern) noexcept { rhythmDrums.setPattern(pattern); }
    [[nodiscard]] bool rhythmEnabled() const noexcept { return rhythmDrums.isEnabled(); }
    [[nodiscard]] int rhythmPattern() const noexcept { return rhythmDrums.currentPattern(); }
    void setStringDrumsEnabled(bool enabled) noexcept { stringDrums.setEnabled(enabled); }
    void setStringDrumsLevel(float level) noexcept { stringDrums.setLevel(level); }
    [[nodiscard]] bool stringDrumsEnabled() const noexcept { return stringDrums.isEnabled(); }
    [[nodiscard]] int lastStringDrum() const noexcept { return stringDrums.lastDetectedString(); }
    void setBassEnabled(bool enabled) noexcept { bassInstrument.setEnabled(enabled); }
    void setBassLevel(float level) noexcept { bassInstrument.setLevel(level); }
    [[nodiscard]] bool bassEnabled() const noexcept { return bassInstrument.isEnabled(); }
    [[nodiscard]] int bassMidiNote() const noexcept { return bassInstrument.bassMidiNote(); }
    [[nodiscard]] float bassFrequencyHz() const noexcept { return bassInstrument.frequencyHz(); }
    void setSynthEnabled(bool enabled) noexcept { synthInstrument.setEnabled(enabled); }
    void setSynthLevel(float level) noexcept { synthInstrument.setLevel(level); }
    void setSynthBrightness(float brightness) noexcept { synthInstrument.setBrightness(brightness); }
    [[nodiscard]] bool synthEnabled() const noexcept { return synthInstrument.isEnabled(); }
    [[nodiscard]] int synthMidiNote() const noexcept { return synthInstrument.midiNote(); }
    [[nodiscard]] float synthFrequencyHz() const noexcept { return synthInstrument.frequencyHz(); }
    void setPianoEnabled(bool enabled) noexcept { pianoInstrument.setEnabled(enabled); }
    void setPianoLevel(float level) noexcept { pianoInstrument.setLevel(level); }
    void setPianoBrightness(float brightness) noexcept { pianoInstrument.setBrightness(brightness); }
    [[nodiscard]] bool pianoEnabled() const noexcept { return pianoInstrument.isEnabled(); }
    [[nodiscard]] int pianoHeldNoteCount() const noexcept { return pianoInstrument.heldNoteCount(); }
    void setViolinEnabled(bool enabled) noexcept { violinInstrument.setEnabled(enabled); }
    void setViolinLevel(float level) noexcept { violinInstrument.setLevel(level); }
    void setViolinBrightness(float brightness) noexcept { violinInstrument.setBrightness(brightness); }
    [[nodiscard]] bool violinEnabled() const noexcept { return violinInstrument.isEnabled(); }
    [[nodiscard]] int violinMidiNote() const noexcept { return violinInstrument.midiNote(); }
    [[nodiscard]] float violinFrequencyHz() const noexcept { return violinInstrument.frequencyHz(); }
    void setInputGainDb(float db) noexcept;
    void setOutputGainDb(float db) noexcept;
    bool requestLoopCommand(LoopCommand) noexcept;
    [[nodiscard]] LoopStatus loopStatus() const noexcept;
    void setLoopLevel(float level) noexcept { loopLevel.store(dsp::safeParameter(level,0,1,1)); }
    [[nodiscard]] std::array<float,128> loopWaveform() const noexcept;
    bool consumeLoopError() noexcept {return loopError.exchange(false);}

private:
    void applyLoopCommands() noexcept;
    void publishLoopStatus() noexcept;
    std::array<LoopCommand,32> loopCommands{};
    std::atomic<unsigned> commandWrite{0}, commandRead{0};
    std::atomic<looper::LooperState> publishedLoopState{looper::LooperState::empty};
    std::atomic<std::size_t> publishedRecorded{0}, publishedLength{0}, publishedPosition{0};
    std::atomic<bool> publishedUndo{false}, publishedRedo{false};
    std::atomic<float> loopLevel{1};
    std::array<std::atomic<float>,1200> waveformPeaks{};
    std::atomic<bool> loopError{false};
    std::atomic<bool> fxBypass { false };
    std::atomic<bool> muted { true };
    std::atomic<float> inputGain { 1.0f };
    std::atomic<float> outputGain { 1.0f };
    SignalMetrics signalMetrics;
    dsp::OutputProtector outputProtector;
    dsp::GuitarRack guitarRack;
    nam::NamProcessor namProcessor;
    ir::IrProcessor irProcessor;
    TempoService tempoService;
    dsp::Metronome metronome;
    dsp::RhythmDrumMachine rhythmDrums;
    dsp::GuitarDrumTrigger stringDrums;
    analysis::TunerTap tunerCapture;
    analysis::MusicalAnalysisEngine musicalAnalyzer;
    analysis::FretboardNoteSolver noteSolver;
    analysis::PolyphonicNoteDetector polyphonicDetector;
    performance::PerformanceEventEngine performanceEvents;
    instruments::BassInstrumentEngine bassInstrument;
    instruments::SynthInstrumentEngine synthInstrument;
    instruments::PianoInstrumentEngine pianoInstrument;
    instruments::ViolinInstrumentEngine violinInstrument;
    looper::LooperEngine looperEngine;
    recording::QuickRecorder quickRecorder;
    std::vector<float> loopScratchLeft;
    std::vector<float> loopScratchRight;
    std::vector<float> metronomeScratch;
    std::vector<float> rhythmScratch;
    std::vector<float> stringDrumScratch;
    std::vector<float> bassScratch;
    std::vector<float> synthScratch;
    std::vector<float> pianoScratch;
    std::vector<float> violinScratch;
    std::atomic<double> preparedSampleRate { 0.0 };
    std::atomic<int> preparedBlockSize { 0 };
};
} // namespace pmx::audio
