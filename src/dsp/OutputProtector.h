#pragma once

namespace pmx::dsp
{
class OutputProtector final
{
public:
    [[nodiscard]] float processSample(float sample) const noexcept;
    void process(float* const* outputs, int numOutputs, int numSamples) const noexcept;
    static constexpr float ceiling = 0.98f;
};
} // namespace pmx::dsp
