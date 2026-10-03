#pragma once
#include <atomic>
#include <vector>
#include "AudioSink.h"
#include "SignalMetrics.h"
#include "TempoService.h"
#include "analysis/TunerTap.h"
#include "dsp/OutputProtector.h"
#include "dsp/GuitarRack.h"
#include "dsp/Metronome.h"
#include "ir/IrProcessor.h"
#include "nam/NamProcessor.h"
#include "looper/LooperEngine.h"
#include "recording/QuickRecorder.h"

namespace pmx::audio
{
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
    looper::LooperEngine& looper() noexcept { return looperEngine; }
    const looper::LooperEngine& looper() const noexcept { return looperEngine; }
    recording::QuickRecorder& recorder() noexcept { return quickRecorder; }
    const recording::QuickRecorder& recorder() const noexcept { return quickRecorder; }
    void setMetronomeEnabled(bool enabled) noexcept { metronome.setEnabled(enabled); }
    void setMetronomeLevel(float level) noexcept { metronome.setLevel(level); }
    void setInputGainDb(float db) noexcept;
    void setOutputGainDb(float db) noexcept;

private:
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
    analysis::TunerTap tunerCapture;
    looper::LooperEngine looperEngine;
    recording::QuickRecorder quickRecorder;
    std::vector<float> loopScratchLeft;
    std::vector<float> loopScratchRight;
    std::vector<float> metronomeScratch;
    double preparedSampleRate { 0.0 };
    int preparedBlockSize { 0 };
};
} // namespace pmx::audio
