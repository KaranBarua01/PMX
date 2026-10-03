#pragma once
#include <string>

namespace pmx::analysis
{
struct TunerResult
{
    double frequencyHz { 0.0 };
    std::string noteName;
    float cents { 0.0f };
    float confidence { 0.0f };
};

class TunerEngine final
{
public:
    [[nodiscard]] TunerResult analyse(const float* samples, int numSamples, double sampleRate) const;
};
} // namespace pmx::analysis
