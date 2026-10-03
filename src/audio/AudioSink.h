#pragma once

namespace pmx::audio
{
class AudioSink
{
public:
    virtual ~AudioSink() = default;
    virtual void process(const float* const* inputs, int numInputs,
                         float* const* outputs, int numOutputs,
                         int numSamples) noexcept = 0;
    virtual void prepare(double sampleRate, int maxBlockSize, int inputChannels, int outputChannels) = 0;
    virtual void stopped() noexcept = 0;
};
} // namespace pmx::audio
