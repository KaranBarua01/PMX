#include <cmath>
#include "analysis/FretboardNoteSolver.h"

namespace
{
pmx::analysis::MusicalAnalysisSnapshot pitch(double hz,
                                             std::uint64_t pitchSerial,
                                             std::uint64_t onsetSerial=1,
                                             float confidence=0.9f)
{
    pmx::analysis::MusicalAnalysisSnapshot value;
    value.noteActive=true;
    value.frequencyHz=hz;
    value.confidence=confidence;
    value.onsetSerial=onsetSerial;
    value.pitchSerial=pitchSerial;
    return value;
}
}

int main()
{
    using pmx::analysis::FretboardNoteSolver;
    using pmx::instruments::GuitarProfile;

    FretboardNoteSolver solver;
    solver.setProfile(GuitarProfile::standard());
    solver.reset();

    solver.process(pitch(109.3,1));
    auto a=solver.snapshot();
    if(!a.noteActive || a.midiNote!=45) return 1;
    if(std::abs(a.targetFrequencyHz-110.0)>0.2) return 2;

    // Small tuning jitter must remain A2 rather than flicker.
    solver.process(pitch(110.6,2));
    solver.process(pitch(109.5,3));
    if(solver.snapshot().midiNote!=45) return 3;

    // During a slide, preserve the current note and expose continuous offset.
    solver.process(pitch(113.5,4));
    const auto sliding=solver.snapshot();
    if(sliding.midiNote!=45) return 4;
    if(!sliding.transitioning || sliding.centsFromTarget<40.0f) return 5;

    // A new semitone must be confirmed twice before changing the stable note.
    solver.process(pitch(116.5409,5));
    if(solver.snapshot().midiNote!=45) return 6;
    solver.process(pitch(116.5409,6));
    const auto changed=solver.snapshot();
    if(changed.midiNote!=46) return 7;
    if(std::abs(changed.centsFromTarget)>2.0f) return 8;

    // Release clears the resolved note.
    auto released=pitch(0.0,6);
    released.noteActive=false;
    released.releaseSerial=1;
    solver.process(released);
    if(solver.snapshot().noteActive || solver.snapshot().midiNote!=-1) return 9;

    // Standard guitar should reject pitches below its playable fretboard.
    solver.reset();
    solver.process(pitch(73.4162,1));
    if(solver.snapshot().midiNote!=-1) return 10;

    // Learned Drop-D tuning makes D2 a valid note without identifying a physical string.
    auto dropD=GuitarProfile::standard();
    if(!dropD.setOpenString(0,73.4162)) return 11;
    solver.setProfile(dropD);
    solver.reset();
    solver.process(pitch(73.4162,1));
    const auto d=solver.snapshot();
    if(d.midiNote!=38) return 12;
    if(std::abs(d.targetFrequencyHz-73.4162)>0.1) return 13;

    // A fresh onset can jump directly to a distant note without inheriting old hysteresis.
    solver.process(pitch(146.832,2,2));
    const auto newOnset=solver.snapshot();
    if(newOnset.midiNote!=50) return 14;

    return 0;
}
