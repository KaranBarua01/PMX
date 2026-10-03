#pragma once
namespace pmx::dsp { class Drive { public: void setAmount(float v) noexcept { target=v<0?0:(v>1?1:v); } void process(float* b,int n) noexcept; private: float current{0.0f},target{0.0f}; }; }
