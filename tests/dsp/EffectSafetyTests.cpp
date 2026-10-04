#include "dsp/effects/Eq.h"
#include <cmath>
#include <iostream>
#include <limits>
#include "dsp/effects/Gate.h"
#include "dsp/effects/Drive.h"
#include "dsp/effects/Compressor.h"
int main()
{
    pmx::dsp::Eq eq;
    eq.prepare(48000);
    float samples[]{0.1f, -0.2f, 0.3f, -0.4f, 0.0f};
    const float expected[]{0.1f, -0.2f, 0.3f, -0.4f, 0.0f};
    eq.process(samples, 5);
    for (int i=0; i<5; ++i)
        if (std::abs(samples[i]-expected[i]) > 0.00001f)
        { std::cerr << "A flat EQ must preserve the input at sample " << i << '\n'; return 1; }
    eq.setLowDb(std::numeric_limits<float>::quiet_NaN());
    eq.setMidDb(std::numeric_limits<float>::infinity());
    float safe[]{0.2f, -0.3f};
    eq.process(safe,2);
    for (auto v:safe) if (!std::isfinite(v)) return 2;

    // A noise gate must follow an envelope, rather than chop every zero crossing.
    pmx::dsp::Gate gate;
    gate.setThresholdDb(-30.0f);
    float tone[256];
    for (int i=0;i<256;++i) tone[i]=0.2f*std::sin(2*3.14159265f*440*i/48000);
    gate.process(tone,256);
    if (tone[109] == 0.0f) { std::cerr << "Gate chopped a sustained note near a zero crossing\n"; return 3; }
    pmx::dsp::Compressor compressor;
    compressor.setThresholdDb(-24);
    float sustained[2048];
    for(auto& v:sustained) v=0.5f;
    compressor.process(sustained,2048);
    if(sustained[0]<=sustained[2047]+0.1f) return 4;
    pmx::dsp::Drive drive;
    drive.setAmount(0);
    float clean[]{0.2f,-0.3f};
    drive.process(clean,2);
    if(std::abs(clean[0]-0.2f)>0.0001f) return 5;
    return 0;
}
