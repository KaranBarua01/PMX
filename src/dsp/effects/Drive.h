#pragma once
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp { class Drive { public: void prepare(double sr) noexcept {sampleRate=sr;lowState=0;current=0;} void setAmount(float v) noexcept { target=safeParameter(v,0,1,0); } void setTone(float v) noexcept {tone=safeParameter(v,0,1,0.5f);} void setOutputDb(float v) noexcept {outputDb=safeParameter(v,-24,6,0);} void process(float*,int) noexcept; private: double sampleRate{48000}; float current{},lowState{}; std::atomic<float> target{0},tone{0.5f},outputDb{0}; }; }
