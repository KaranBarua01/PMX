#pragma once
#include <vector>
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp { class Delay { public: void prepare(double sr,int maxBlock,float maxDelayMs); void setTimeMs(float v) noexcept { timeMs=safeParameter(v,1,2000,350); } void setFeedback(float v) noexcept { feedback=safeParameter(v,0,0.95f,0.25f); } void setMix(float v) noexcept { mix=safeParameter(v,0,1,0.18f); } void process(float*,int) noexcept; private: std::vector<float> ring; double sampleRate{44100}; std::atomic<float> timeMs{350},feedback{0.25f},mix{0.18f}; int write{}; }; }
