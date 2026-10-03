#pragma once
#include <vector>
namespace pmx::dsp { class Delay { public: void prepare(double sr,int maxBlock,float maxDelayMs); void setTimeMs(float v) noexcept { timeMs=v; } void setFeedback(float v) noexcept { feedback=v<0?0:(v>0.95f?0.95f:v); } void setMix(float v) noexcept { mix=v<0?0:(v>1?1:v); } void process(float* b,int n) noexcept; private: std::vector<float> ring; double sampleRate{44100}; float timeMs{350},feedback{0.25f},mix{0.18f}; int write{0}; }; }
