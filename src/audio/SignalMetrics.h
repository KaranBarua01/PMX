#pragma once
#include <atomic>

namespace pmx::audio
{
struct SignalMetricsSnapshot
{
    float inputPeak { 0.0f };
    float outputPeak { 0.0f };
};

class SignalMetrics final
{
public:
    void updateInput(float peak) noexcept;
    void updateOutput(float peak) noexcept;
    [[nodiscard]] SignalMetricsSnapshot snapshot() const noexcept;
    void reset() noexcept;
private:
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
};
} // namespace pmx::audio
