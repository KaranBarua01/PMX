#pragma once
namespace pmx::dsp { class Gate { public: void setThresholdDb(float db) noexcept { thresholdDb=db; } void process(float* b,int n) noexcept; private: float thresholdDb{-60.0f}; }; }
