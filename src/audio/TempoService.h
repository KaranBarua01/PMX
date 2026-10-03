#pragma once
#include <array>
#include <atomic>

namespace pmx::audio
{
class TempoService final
{
public:
    void setBpm(double newBpm) noexcept;
    [[nodiscard]] double bpm() const noexcept { return value.load(std::memory_order_relaxed); }
    void tap(double timestampSeconds) noexcept;
    void resetTapHistory() noexcept;
    [[nodiscard]] double samplesPerBeat(double sampleRate) const noexcept;
private:
    std::atomic<double> value { 120.0 };
    std::array<double, 4> intervals {};
    int intervalCount { 0 };
    int intervalWrite { 0 };
    double lastTap { 0.0 };
    bool hasLastTap { false };
};
} // namespace pmx::audio
