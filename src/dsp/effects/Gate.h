#pragma once
#include <atomic>
#include "Parameter.h"
namespace pmx::dsp
{
class Gate final
{
public:
    void prepare(double sr) noexcept { sampleRate=sr; envelope=0; gain=0; }
    void setThresholdDb(float db) noexcept { thresholdDb=safeParameter(db,-90,0,-60); }
    void setAttackMs(float v) noexcept { attackMs=safeParameter(v,0.1f,50,3); }
    void setReleaseMs(float v) noexcept { releaseMs=safeParameter(v,10,1000,120); }
    void process(float*, int) noexcept;
private:
    double sampleRate{48000};
    float envelope{}, gain{};
    std::atomic<float> thresholdDb{-60}, attackMs{3}, releaseMs{120};
};
}
