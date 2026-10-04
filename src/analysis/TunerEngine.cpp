#include "TunerEngine.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace pmx::analysis
{
TunerResult TunerEngine::analyse(const float* x, int n, double sr) const
{
    if (x == nullptr || n < 16 || sr <= 0.0) return {};

    double sumSq = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        if (! std::isfinite(x[i])) continue;
        sumSq += static_cast<double>(x[i]) * x[i];
        peak = std::max(peak, std::abs(x[i]));
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(n));
    if (rms < 0.002 || peak < 0.005f) return {};

    // YIN cumulative difference detects the fundamental despite guitar harmonics.
    // This analyser runs outside the audio callback, with a bounded 4096-sample window.
    if (n>4096) { x+=n-4096; n=4096; }
    for (int i=0;i<n;++i) if (!std::isfinite(x[i])) return {};
    const int minLag=std::max(2,static_cast<int>(sr/1400.0));
    const int maxLag=std::min(n/2-1,static_cast<int>(sr/45.0));
    if (maxLag<=minLag) return {};
    std::vector<double> difference(static_cast<std::size_t>(maxLag+1),1.0);
    double cumulative=0;
    for (int lag=1;lag<=maxLag;++lag)
    {
        double sum=0;
        for(int i=0;i<n/2;++i) { const double delta=x[i]-x[i+lag]; sum+=delta*delta; }
        cumulative+=sum;
        difference[static_cast<std::size_t>(lag)]=cumulative>1e-12?sum*lag/cumulative:1.0;
    }
    int selected=0;
    for(int lag=minLag;lag<maxLag;++lag)
        if(difference[static_cast<std::size_t>(lag)]<0.15)
        {
            while(lag<maxLag && difference[static_cast<std::size_t>(lag+1)]<difference[static_cast<std::size_t>(lag)]) ++lag;
            selected=lag; break;
        }
    if(selected==0 || selected>=maxLag) return {};
    const double a=difference[static_cast<std::size_t>(selected-1)], b=difference[static_cast<std::size_t>(selected)], c=difference[static_cast<std::size_t>(selected+1)];
    const double denominator=a-2*b+c;
    const double correction=std::abs(denominator)>1e-12?std::clamp(0.5*(a-c)/denominator,-0.5,0.5):0;
    const double frequency = sr / (selected+correction);
    if (frequency<45 || frequency>1400) return {};
    const double midiExact = 69.0 + 12.0 * std::log2(frequency / 440.0);
    const int midi = static_cast<int>(std::lround(midiExact));
    static constexpr const char* names[] {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const int noteIndex = ((midi % 12) + 12) % 12;
    const int octave = midi / 12 - 1;
    const double target = 440.0 * std::pow(2.0, (midi - 69) / 12.0);
    const float cents = static_cast<float>(1200.0 * std::log2(frequency / target));
    const float pitchConfidence = static_cast<float>(std::clamp(1.0-b,0.0,1.0));
    const float levelConfidence = static_cast<float>(std::clamp(rms / 0.05, 0.0, 1.0));

    TunerResult result;
    result.frequencyHz = frequency;
    result.noteName = std::string(names[noteIndex]) + std::to_string(octave);
    result.cents = cents;
    result.confidence = pitchConfidence * levelConfidence;
    return result;
}
} // namespace pmx::analysis
