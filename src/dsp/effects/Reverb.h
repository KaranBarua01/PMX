#pragma once
#include <vector>
namespace pmx::dsp { class Reverb { public: void prepare(double sr); void setMix(float v) noexcept { mix=v; } void setDecay(float v) noexcept { decay=v<0?0:(v>0.92f?0.92f:v); } void process(float* b,int n) noexcept; private: std::vector<float> ring; float mix{0.18f},decay{0.65f}; int write{0}; }; }
