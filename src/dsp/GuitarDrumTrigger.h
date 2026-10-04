#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace pmx::dsp
{
enum class GuitarDrumPad : int
{
    lowE = 0,
    a = 1,
    d = 2,
    g = 3,
    b = 4,
    highE = 5,
    none = -1
};

class GuitarDrumTrigger final
{
public:
    void prepare(double sampleRate) noexcept;
    void process(const float* input, float* monoOut, int numSamples) noexcept;

    void setEnabled(bool value) noexcept;
    void setLevel(float value) noexcept { level.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed); }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled.load(std::memory_order_relaxed); }
    [[nodiscard]] int lastDetectedString() const noexcept { return lastDetected.load(std::memory_order_relaxed); }

private:
    static constexpr int captureSize = 1024;
    GuitarDrumPad classifyCapturedPitch() const noexcept;
    float correlationAtLag(int lag) const noexcept;
    void trigger(GuitarDrumPad pad) noexcept;
    float noise() noexcept;
    float renderVoice() noexcept;

    double sampleRate { 44100.0 };
    std::array<float, captureSize> capture {};
    int captureCount {};
    int cooldownSamples {};
    float slowEnvelope {};
    float previousAbs {};
    bool wasEnabled {};

    float kickEnvelope {}, snareEnvelope {}, closedHatEnvelope {}, openHatEnvelope {}, tomEnvelope {}, crashEnvelope {};
    double kickPhase {}, snarePhase {}, tomPhase {}, crashPhase {};
    float previousNoise {};
    std::uint32_t randomState { 0x725341u };

    std::atomic<bool> enabled { false };
    std::atomic<float> level { 0.55f };
    std::atomic<int> lastDetected { -1 };
};
} // namespace pmx::dsp
