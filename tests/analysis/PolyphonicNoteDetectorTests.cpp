#include <array>
#include <cmath>
#include <cstdint>
#include "analysis/PolyphonicNoteDetector.h"

namespace
{
constexpr double pi=3.14159265358979323846;
constexpr double sampleRate=44100.0;

bool containsMidi(const pmx::analysis::PolyphonicSnapshot& snapshot,int midi)
{
    for(int i=0;i<snapshot.noteCount;++i)
        if(snapshot.notes[static_cast<std::size_t>(i)].midiNote==midi)return true;
    return false;
}

template <typename Generator>
void feed(pmx::analysis::PolyphonicNoteDetector& detector,int samples,Generator generator)
{
    std::array<float,128> block{};
    int position=0;
    while(position<samples)
    {
        const int count=std::min(128,samples-position);
        for(int i=0;i<count;++i)block[static_cast<std::size_t>(i)]=generator(position+i);
        detector.process(block.data(),count);
        position+=count;
    }
}
}

int main()
{
    using pmx::analysis::PolyphonicNoteDetector;
    using pmx::instruments::GuitarProfile;

    PolyphonicNoteDetector detector;
    detector.prepare(sampleRate);
    detector.setProfile(GuitarProfile::standard());
    detector.reset();

    // A harmonic-rich single E2 must remain one musical note rather than
    // reporting its E3/B3 overtones as extra played notes.
    feed(detector,8192,[](int sample)
    {
        const double t=static_cast<double>(sample)/sampleRate;
        return static_cast<float>(
            0.12*std::sin(2.0*pi*82.4069*t)+
            0.08*std::sin(2.0*pi*164.8138*t)+
            0.04*std::sin(2.0*pi*247.2207*t));
    });

    auto single=detector.snapshot();
    if(single.noteCount!=1)return 1;
    if(!containsMidi(single,40))return 2;

    // A C major triad should survive the same harmonic rejection as three
    // distinct musical notes. Exact physical strings are intentionally irrelevant.
    detector.reset();
    feed(detector,8192,[](int sample)
    {
        const double t=static_cast<double>(sample)/sampleRate;
        constexpr int midiNotes[]{48,52,55};
        double value=0.0;
        for(const int midi:midiNotes)
        {
            const double frequency=440.0*std::pow(2.0,static_cast<double>(midi-69)/12.0);
            value+=0.08*std::sin(2.0*pi*frequency*t);
            value+=0.025*std::sin(2.0*pi*frequency*2.0*t);
            value+=0.012*std::sin(2.0*pi*frequency*3.0*t);
        }
        return static_cast<float>(value);
    });

    const auto chord=detector.snapshot();
    if(chord.noteCount!=3)return 3;
    if(!containsMidi(chord,48))return 4;
    if(!containsMidi(chord,52))return 5;
    if(!containsMidi(chord,55))return 6;

    // Silence clears a previously stable chord.
    feed(detector,8192,[](int){return 0.0f;});
    if(detector.snapshot().noteCount!=0)return 7;

    // Learned Drop-D extends the playable candidate range without requiring
    // physical-string identity.
    auto dropD=GuitarProfile::standard();
    if(!dropD.setOpenString(0,73.4162))return 8;
    detector.setProfile(dropD);
    detector.reset();

    feed(detector,8192,[](int sample)
    {
        const double t=static_cast<double>(sample)/sampleRate;
        return static_cast<float>(0.11*std::sin(2.0*pi*73.4162*t));
    });

    const auto d=detector.snapshot();
    if(d.noteCount!=1)return 9;
    if(!containsMidi(d,38))return 10;

    return 0;
}
