#pragma once
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp { class Eq { public: void prepare(double sr) noexcept { sampleRate=sr; lowState=highState=0; } void setLowDb(float v) noexcept { lowDb=safeParameter(v,-18,18,0); } void setMidDb(float v) noexcept { midDb=safeParameter(v,-18,18,0); } void setHighDb(float v) noexcept { highDb=safeParameter(v,-18,18,0); } void process(float* b,int n) noexcept; private: double sampleRate{44100.0}; float lowState{},highState{}; std::atomic<float> lowDb{0},midDb{0},highDb{0}; }; }
