#include <algorithm>
#include <array>
#include <cmath>
#include "audio/TempoService.h"
#include "dsp/RhythmDrumMachine.h"

int main()
{
    pmx::audio::TempoService tempo;
    tempo.setBpm(120.0);
    pmx::dsp::RhythmDrumMachine drums(tempo);
    drums.prepare(1000.0);

    std::array<float, 1000> buffer {};
    drums.process(buffer.data(), static_cast<int>(buffer.size()));
    if (std::any_of(buffer.begin(), buffer.end(), [](float v){ return v != 0.0f; })) return 1;

    drums.setPattern(99);
    if (drums.currentPattern() != pmx::dsp::RhythmDrumMachine::patternCount - 1) return 2;
    drums.setPattern(-4);
    if (drums.currentPattern() != 0) return 3;

    drums.setPattern(1);
    drums.setLevel(0.5f);
    drums.setEnabled(true);
    drums.process(buffer.data(), static_cast<int>(buffer.size()));

    float peak = 0.0f;
    for (float value : buffer)
    {
        if (!std::isfinite(value)) return 4;
        peak = std::max(peak, std::abs(value));
    }
    if (peak <= 0.01f) return 5;

    buffer.fill(0.0f);
    drums.setLevel(0.0f);
    drums.process(buffer.data(), static_cast<int>(buffer.size()));
    if (std::any_of(buffer.begin(), buffer.end(), [](float v){ return v != 0.0f; })) return 6;

    drums.setEnabled(false);
    drums.setLevel(1.0f);
    buffer.fill(0.0f);
    drums.process(buffer.data(), static_cast<int>(buffer.size()));
    if (std::any_of(buffer.begin(), buffer.end(), [](float v){ return v != 0.0f; })) return 7;

    return 0;
}
