#pragma once
namespace pmx::dsp { class Compressor { public: void setThresholdDb(float v) noexcept { thresholdDb=v; } void setRatio(float v) noexcept { ratio=v<1.0f?1.0f:v; } void process(float* b,int n) noexcept; private: float thresholdDb{-18.0f}, ratio{4.0f}; }; }
