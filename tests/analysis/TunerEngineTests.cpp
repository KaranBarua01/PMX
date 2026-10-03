#include <cmath>
#include <vector>
#include "analysis/TunerEngine.h"

static std::vector<float> sine(double frequency, double sampleRate, int samples)
{
    std::vector<float> out(static_cast<size_t>(samples));
    for (int i = 0; i < samples; ++i)
        out[static_cast<size_t>(i)] = 0.7f * std::sin(2.0 * 3.141592653589793 * frequency * i / sampleRate);
    return out;
}

int main()
{
    pmx::analysis::TunerEngine tuner;
    constexpr double sr = 48000.0;

    for (const auto expected : {82.41, 110.0, 440.0})
    {
        auto tone = sine(expected, sr, 24000);
        auto result = tuner.analyse(tone.data(), static_cast<int>(tone.size()), sr);
        if (result.confidence < 0.8f) return 1;
        if (std::abs(result.frequencyHz - expected) > 0.7) return 2;
        if (result.noteName.empty()) return 3;
    }

    std::vector<float> silence(4096, 0.0f);
    auto none = tuner.analyse(silence.data(), static_cast<int>(silence.size()), sr);
    if (none.confidence != 0.0f) return 4;
    if (none.frequencyHz != 0.0) return 5;
    return 0;
}
