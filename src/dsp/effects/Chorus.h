#pragma once
#include <vector>
namespace pmx::dsp { class Chorus { public: void prepare(double sr); void setRateHz(float v) noexcept { rateHz=v; } void setDepth(float v) noexcept { depth=v; } void setMix(float v) noexcept { mix=v; } void process(float* b,int n) noexcept; private: std::vector<float> ring; double sampleRate{44100}; float rateHz{0.8f},depth{0.35f},mix{0.25f},phase{0}; int write{0}; }; }
