#pragma once
#include <algorithm>
#include <cmath>
namespace pmx::dsp
{
inline float safeParameter(float value, float lo, float hi, float fallback) noexcept
{ return std::isfinite(value) ? std::clamp(value,lo,hi) : fallback; }
}
