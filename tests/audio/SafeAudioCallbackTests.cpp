#include <cmath>
#include <limits>
#include "audio/SafeAudioCallback.h"

int main()
{
    float left[] { 0.25f, std::numeric_limits<float>::quiet_NaN(), 0.5f };
    float right[] { std::numeric_limits<float>::infinity(), -0.4f, 0.1f };
    float* channels[] { left, right };

    pmx::audio::SafeAudioCallback::sanitize(channels, 2, 3);

    if (left[0] != 0.25f || left[1] != 0.0f || left[2] != 0.5f) return 1;
    if (right[0] != 0.0f || right[1] != -0.4f || right[2] != 0.1f) return 2;
    return 0;
}
