#include "TempoService.h"
#include <algorithm>

namespace pmx::audio
{
void TempoService::setBpm(double newBpm) noexcept
{
    value.store(std::clamp(newBpm, 30.0, 300.0), std::memory_order_relaxed);
}

void TempoService::resetTapHistory() noexcept
{
    intervals.fill(0.0);
    intervalCount = intervalWrite = 0;
    hasLastTap = false;
    lastTap = 0.0;
}

void TempoService::tap(double now) noexcept
{
    if (! hasLastTap)
    {
        lastTap = now;
        hasLastTap = true;
        return;
    }
    const double interval = now - lastTap;
    lastTap = now;
    if (interval <= 0.0 || interval > 2.0)
    {
        intervalCount = intervalWrite = 0;
        return;
    }
    if (interval < 0.15) return;
    intervals[static_cast<size_t>(intervalWrite)] = interval;
    intervalWrite = (intervalWrite + 1) % static_cast<int>(intervals.size());
    intervalCount = std::min(intervalCount + 1, static_cast<int>(intervals.size()));
    double sum = 0.0;
    for (int i = 0; i < intervalCount; ++i) sum += intervals[static_cast<size_t>(i)];
    setBpm(60.0 / (sum / intervalCount));
}

double TempoService::samplesPerBeat(double sampleRate) const noexcept
{
    return sampleRate * 60.0 / bpm();
}
} // namespace pmx::audio
