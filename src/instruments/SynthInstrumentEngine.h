#pragma once
#include <atomic>
#include "performance/PerformanceEventEngine.h"

namespace pmx::instruments
{
class SynthInstrumentEngine final : public performance::PerformanceEventSink
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setEnabled(bool shouldEnable) noexcept { enabled.store(shouldEnable,std::memory_order_relaxed); }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled.load(std::memory_order_relaxed); }
    void setLevel(float newLevel) noexcept;
    [[nodiscard]] float level() const noexcept { return outputLevel.load(std::memory_order_relaxed); }
    void setBrightness(float newBrightness) noexcept;
    [[nodiscard]] float brightness() const noexcept { return toneBrightness.load(std::memory_order_relaxed); }
    void handlePerformanceEvent(const performance::PerformanceEvent& event) noexcept override;
    void render(float* output,int numSamples) noexcept;
    [[nodiscard]] bool noteActive() const noexcept { return publishedActive.load(std::memory_order_relaxed); }
    [[nodiscard]] int midiNote() const noexcept { return publishedMidi.load(std::memory_order_relaxed); }
    [[nodiscard]] float frequencyHz() const noexcept { return publishedFrequency.load(std::memory_order_relaxed); }
private:
    static double midiFrequency(int midi,float cents) noexcept;
    std::atomic<bool> enabled { false };
    std::atomic<float> outputLevel { 0.55f };
    std::atomic<float> toneBrightness { 0.55f };
    std::atomic<bool> publishedActive { false };
    std::atomic<int> publishedMidi { -1 };
    std::atomic<float> publishedFrequency { 0.0f };
    double sampleRate { 48000.0 };
    double phase {};
    double detunedPhase {};
    double frequency {};
    double targetFrequency {};
    float envelope {};
    float targetAmplitude {};
    float bendCents {};
    int currentMidi { -1 };
    bool gate {};
};
} // namespace pmx::instruments
