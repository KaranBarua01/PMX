#pragma once
namespace pmx::dsp { class Eq { public: void prepare(double sr) noexcept { sampleRate=sr; lowState=0; } void setLowDb(float v) noexcept { lowDb=v; } void setMidDb(float v) noexcept { midDb=v; } void setHighDb(float v) noexcept { highDb=v; } void process(float* b,int n) noexcept; private: double sampleRate{44100.0}; float lowState{0},lowDb{0},midDb{0},highDb{0}; }; }
