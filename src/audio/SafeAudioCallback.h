#pragma once

namespace pmx::audio
{
struct SafeAudioCallback final
{
    static void sanitize(float* const* channels, int numChannels, int numSamples) noexcept;
};
} // namespace pmx::audio
