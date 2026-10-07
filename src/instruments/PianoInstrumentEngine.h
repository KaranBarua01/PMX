#pragma once
#include <array>
#include <atomic>
#include "performance/PerformanceEventEngine.h"

namespace pmx::instruments
{
class PianoInstrumentEngine final : public performance::PerformanceEventSink
{
public:
    static constexpr int maxVoices=analysis::PolyphonicSnapshot::maxNotes;

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
    [[nodiscard]] int heldNoteCount() const noexcept { return publishedHeldCount.load(std::memory_order_relaxed); }

private:
    struct Voice
    {
        int midi{-1};
        double phase{};
        double frequency{};
        float envelope{};
        float strike{};
        std::uint64_t ageSamples{};
        bool active{};
        bool keyDown{};
    };

    static double midiFrequency(int midi) noexcept;
    Voice* findVoice(int midi) noexcept;
    Voice* allocateVoice(int midi) noexcept;
    void publishHeldCount() noexcept;

    std::atomic<bool> enabled { false };
    std::atomic<float> outputLevel { 0.55f };
    std::atomic<float> toneBrightness { 0.55f };
    std::atomic<int> publishedHeldCount { 0 };
    std::array<Voice,maxVoices> voices {};
    double sampleRate { 48000.0 };
};
} // namespace pmx::instruments
