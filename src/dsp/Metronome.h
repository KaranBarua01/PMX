#pragma once
#include "audio/TempoService.h"
namespace pmx::dsp
{
class Metronome final
{
public:
    explicit Metronome(audio::TempoService& t):tempo(t){}
    void prepare(double sampleRate) noexcept { sr=sampleRate; phase=0.0; clickRemaining=0; }
    void setEnabled(bool e) noexcept { enabled=e; }
    void setLevel(float l) noexcept { level=l; }
    void process(float* mono,int n) noexcept;
private:
    audio::TempoService& tempo; double sr{44100.0},phase{0.0}; int clickRemaining{0}; bool enabled{false}; float level{0.25f};
};
}
