#include "TunerEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::analysis
{
TunerResult TunerEngine::analyse(const float* x, int n, double sr) const
{
    if (x == nullptr || n < 16 || sr <= 0.0) return {};

    double sumSq = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        if (! std::isfinite(x[i])) continue;
        sumSq += static_cast<double>(x[i]) * x[i];
        peak = std::max(peak, std::abs(x[i]));
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(n));
    if (rms < 0.002 || peak < 0.005f) return {};

    bool haveCrossing = false;
    double previousCrossing = 0.0;
    double periodSum = 0.0;
    int periodCount = 0;
    for (int i = 1; i < n; ++i)
    {
        const float a = x[i - 1], b = x[i];
        if (! std::isfinite(a) || ! std::isfinite(b) || !(a <= 0.0f && b > 0.0f)) continue;
        const double denom = static_cast<double>(b) - a;
        if (std::abs(denom) < 1.0e-12) continue;
        const double crossing = static_cast<double>(i - 1) + (-static_cast<double>(a) / denom);
        if (haveCrossing)
        {
            const double period = crossing - previousCrossing;
            const double frequency = sr / period;
            if (frequency >= 45.0 && frequency <= 1400.0)
            {
                periodSum += period;
                ++periodCount;
            }
        }
        previousCrossing = crossing;
        haveCrossing = true;
    }
    if (periodCount < 2) return {};

    const double frequency = sr / (periodSum / periodCount);
    const double midiExact = 69.0 + 12.0 * std::log2(frequency / 440.0);
    const int midi = static_cast<int>(std::lround(midiExact));
    static constexpr const char* names[] {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const int noteIndex = ((midi % 12) + 12) % 12;
    const int octave = midi / 12 - 1;
    const double target = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    const float cents = static_cast<float>(1200.0 * std::log2(frequency / target));
    const float crossingsConfidence = std::clamp(periodCount / 8.0f, 0.0f, 1.0f);
    const float levelConfidence = static_cast<float>(std::clamp(rms / 0.05, 0.0, 1.0));

    TunerResult result;
    result.frequencyHz = frequency;
    result.noteName = std::string(names[noteIndex]) + std::to_string(octave);
    result.cents = cents;
    result.confidence = crossingsConfidence * levelConfidence;
    return result;
}
} // namespace pmx::analysis
