#pragma once
#include "audio/TempoService.h"
#include "effects/Parameter.h"
namespace pmx::dsp
{
class Metronome final
{
public:
    explicit Metronome(audio::TempoService& t):tempo(t){}
    void prepare(double sampleRate) noexcept { sr=sampleRate; phase=0.0; clickRemaining=0; }
    void setEnabled(bool e) noexcept { enabled=e; }
    void setLevel(float l) noexcept { level=safeParameter(l,0,1,0.25f); }
    void process(float* mono,int n) noexcept;
private:
    audio::TempoService& tempo; double sr{44100.0},phase{0.0}; int clickRemaining{0}; std::atomic<bool> enabled{false}; std::atomic<float> level{0.25f};
};
}
