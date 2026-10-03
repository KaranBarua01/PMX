#include "SignalMetrics.h"

namespace pmx::audio
{
namespace
{
void storePeak(std::atomic<float>& target, float value) noexcept
{
    target.store(value, std::memory_order_relaxed);
}
}
void SignalMetrics::updateInput(float peak) noexcept { storePeak(inputPeak, peak); }
void SignalMetrics::updateOutput(float peak) noexcept { storePeak(outputPeak, peak); }
SignalMetricsSnapshot SignalMetrics::snapshot() const noexcept
{
    return { inputPeak.load(std::memory_order_relaxed), outputPeak.load(std::memory_order_relaxed) };
}
void SignalMetrics::reset() noexcept
{
    inputPeak.store(0.0f, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
}
} // namespace pmx::audio
