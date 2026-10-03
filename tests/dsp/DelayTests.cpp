#include <cmath>
#include "dsp/effects/Delay.h"

int main()
{
    pmx::dsp::Delay delay;
    delay.prepare(1000.0, 64, 100.0);
    delay.setTimeMs(10.0f);
    delay.setFeedback(0.0f);
    delay.setMix(1.0f);

    float buffer[20] {};
    buffer[0] = 1.0f;
    delay.process(buffer, 20);

    if (std::abs(buffer[0]) > 0.0001f) return 1;
    if (std::abs(buffer[9]) > 0.0001f) return 2;
    if (std::abs(buffer[10] - 1.0f) > 0.001f) return 3;
    return 0;
}
