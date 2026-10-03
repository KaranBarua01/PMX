#include <cmath>
#include <limits>
#include "dsp/OutputProtector.h"

int main()
{
    pmx::dsp::OutputProtector p;
    if (p.processSample(std::numeric_limits<float>::quiet_NaN()) != 0.0f) return 1;
    if (p.processSample(std::numeric_limits<float>::infinity()) != 0.0f) return 2;
    if (p.processSample(0.5f) != 0.5f) return 3;
    if (std::abs(p.processSample(4.0f)) > 0.981f) return 4;
    if (std::abs(p.processSample(-4.0f)) > 0.981f) return 5;
    return 0;
}
