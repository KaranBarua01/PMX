#include "OutputProtector.h"
#include <algorithm>
#include <cmath>

namespace pmx::dsp
{
float OutputProtector::processSample(float sample) const noexcept
{
    if (! std::isfinite(sample)) return 0.0f;
    return std::clamp(sample, -ceiling, ceiling);
}

void OutputProtector::process(float* const* outputs, int numOutputs, int numSamples) const noexcept
{
    if (outputs == nullptr) return;
    for (int c = 0; c < numOutputs; ++c)
    {
        auto* out = outputs[c];
        if (out == nullptr) continue;
        for (int i = 0; i < numSamples; ++i) out[i] = processSample(out[i]);
    }
}
} // namespace pmx::dsp
