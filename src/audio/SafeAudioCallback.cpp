#include "SafeAudioCallback.h"
#include <cmath>

namespace pmx::audio
{
void SafeAudioCallback::sanitize(float* const* channels, int numChannels, int numSamples) noexcept
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0) return;
    for (int c = 0; c < numChannels; ++c)
    {
        auto* data = channels[c];
        if (data == nullptr) continue;
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite(data[i])) data[i] = 0.0f;
    }
}
} // namespace pmx::audio
