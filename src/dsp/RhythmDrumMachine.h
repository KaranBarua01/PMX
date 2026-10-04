#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include "audio/TempoService.h"

namespace pmx::dsp
{
class RhythmDrumMachine final
{
public:
    explicit RhythmDrumMachine(audio::TempoService& tempoService) : tempo(tempoService) {}

    void prepare(double sampleRate) noexcept;
    void process(float* mono, int numSamples) noexcept;

    void setEnabled(bool value) noexcept { enabled.store(value, std::memory_order_relaxed); }
    void setLevel(float value) noexcept { level.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed); }
    void setPattern(int value) noexcept { pattern.store(std::clamp(value, 0, patternCount - 1), std::memory_order_relaxed); }

    [[nodiscard]] bool isEnabled() const noexcept { return enabled.load(std::memory_order_relaxed); }
    [[nodiscard]] float currentLevel() const noexcept { return level.load(std::memory_order_relaxed); }
    [[nodiscard]] int currentPattern() const noexcept { return pattern.load(std::memory_order_relaxed); }

    static constexpr int patternCount = 4;

private:
    void triggerStep(int step) noexcept;
    float noise() noexcept;

    audio::TempoService& tempo;
    double sampleRate { 44100.0 };
    double stepPhase {};
    double kickPhase {};
    double snarePhase {};
    float kickEnvelope {};
    float snareEnvelope {};
    float hatEnvelope {};
    float previousNoise {};
    int currentStep {};
    bool wasEnabled {};
    std::uint32_t randomState { 0x31415926u };
    std::atomic<bool> enabled { false };
    std::atomic<float> level { 0.35f };
    std::atomic<int> pattern { 1 };
};
} // namespace pmx::dsp
