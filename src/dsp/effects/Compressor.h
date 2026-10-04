#pragma once
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp { class Compressor { public: void prepare(double sr) noexcept {sampleRate=sr; gain=1;} void setThresholdDb(float v) noexcept { thresholdDb=safeParameter(v,-60,0,-18); } void setRatio(float v) noexcept { ratio=safeParameter(v,1,20,4); } void setMakeupDb(float v) noexcept {makeupDb=safeParameter(v,0,18,0);} void process(float*,int) noexcept; private: double sampleRate{48000}; float gain{1}; std::atomic<float> thresholdDb{-18}, ratio{4}, makeupDb{0}; }; }
