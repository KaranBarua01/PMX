#pragma once
#include <vector>
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp { class Chorus { public: void prepare(double sr); void setRateHz(float v) noexcept { rateHz=safeParameter(v,0.05f,8,0.8f); } void setDepth(float v) noexcept { depth=safeParameter(v,0,1,0.35f); } void setMix(float v) noexcept { mix=safeParameter(v,0,1,0.25f); } void process(float*,int) noexcept; private: std::vector<float> ring; double sampleRate{44100}; std::atomic<float> rateHz{0.8f},depth{0.35f},mix{0.25f}; float phase{}; int write{}; }; }
