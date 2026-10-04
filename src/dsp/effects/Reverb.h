#pragma once
#include <vector>
#include <atomic>
#include <array>
#include "Parameter.h"
namespace pmx::dsp { class Reverb { public: void prepare(double sr); void setMix(float v) noexcept { mix=safeParameter(v,0,1,0.18f); } void setDecay(float v) noexcept { decay=safeParameter(v,0,0.92f,0.65f); } void setTone(float v) noexcept {tone=safeParameter(v,0,1,0.6f);} void process(float*,int) noexcept; private: std::array<std::vector<float>,4> rings; std::array<std::size_t,4> write{}; std::array<float,4> filtered{}; std::atomic<float> mix{0.18f},decay{0.65f},tone{0.6f}; }; }
