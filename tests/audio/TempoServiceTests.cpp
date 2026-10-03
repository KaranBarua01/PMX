#include <cmath>
#include "audio/TempoService.h"

int main()
{
    pmx::audio::TempoService tempo;
    tempo.tap(0.0);
    tempo.tap(0.5);
    tempo.tap(1.0);
    tempo.tap(1.5);
    if (std::abs(tempo.bpm() - 120.0) > 0.01) return 1;
    if (std::abs(tempo.samplesPerBeat(48000.0) - 24000.0) > 1.0) return 2;

    tempo.resetTapHistory();
    tempo.tap(0.0);
    tempo.tap(2.0 / 3.0);
    tempo.tap(4.0 / 3.0);
    if (std::abs(tempo.bpm() - 90.0) > 0.1) return 3;

    tempo.setBpm(500.0);
    if (tempo.bpm() != 300.0) return 4;
    tempo.setBpm(10.0);
    if (tempo.bpm() != 30.0) return 5;
    return 0;
}
