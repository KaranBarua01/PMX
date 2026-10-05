#include "GuitarCalibration.h"
#include <algorithm>
#include <cmath>

namespace pmx::instruments
{
namespace
{
bool validReading(const analysis::TunerResult& result) noexcept
{
    return std::isfinite(result.frequencyHz)
        && result.frequencyHz >= 40.0
        && result.frequencyHz <= 1400.0
        && std::isfinite(result.confidence);
}

double centsBetween(double a, double b) noexcept
{
    if (a <= 0.0 || b <= 0.0) return 10000.0;
    return 1200.0 * std::abs(std::log2(a / b));
}
}

void GuitarCalibration::start(const GuitarProfile& seed) noexcept
{
    learnedProfile = seed.valid() ? seed : GuitarProfile::standard();
    readings.fill(0.0);
    readingCount = 0;
    stringIndex = 0;
    isActive = true;
    isCompleted = false;
}

void GuitarCalibration::cancel() noexcept
{
    readings.fill(0.0);
    readingCount = 0;
    stringIndex = 0;
    isActive = false;
    isCompleted = false;
}

bool GuitarCalibration::submit(const analysis::TunerResult& result) noexcept
{
    if (!isActive || !validReading(result) || result.confidence < minimumConfidence)
        return false;

    if (readingCount > 0 && centsBetween(result.frequencyHz, readings[static_cast<std::size_t>(readingCount - 1)]) > stableCents)
    {
        readings.fill(0.0);
        readingCount = 0;
    }

    readings[static_cast<std::size_t>(readingCount++)] = result.frequencyHz;
    if (readingCount < readingsNeeded) return false;

    auto sorted = readings;
    std::sort(sorted.begin(), sorted.end());
    const auto learnedFrequency = (sorted[1] + sorted[2]) * 0.5;
    learnedProfile.setOpenString(static_cast<std::size_t>(stringIndex), learnedFrequency);

    readings.fill(0.0);
    readingCount = 0;
    ++stringIndex;
    if (stringIndex >= static_cast<int>(GuitarProfile::stringCount))
    {
        isActive = false;
        isCompleted = true;
    }
    return true;
}
} // namespace pmx::instruments
