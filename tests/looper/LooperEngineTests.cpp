#include <array>
#include <cmath>
#include "looper/LooperEngine.h"
#include "looper/LoopExportService.h"
#include <filesystem>

static bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }

int main()
{
    using pmx::looper::LooperEngine;
    using pmx::looper::LooperState;

    LooperEngine looper;
    looper.prepare(10.0, 2.0); // 20-frame capacity, stereo
    if (looper.state() != LooperState::empty) return 1;
    if (looper.capacityFrames() != 20) return 2;

    std::array<float,4> inL{0.1f,0.2f,0.3f,0.4f};
    std::array<float,4> inR{0.4f,0.3f,0.2f,0.1f};
    std::array<float,4> outL{}, outR{};

    if (!looper.record()) return 3;
    looper.process(inL.data(), inR.data(), outL.data(), outR.data(), 4);
    if (looper.recordedFrames() != 4) return 4;
    if (!looper.play()) return 5; // finalise first pass and enter playback
    if (looper.state() != LooperState::playing || looper.loopFrames() != 4) return 6;

    outL.fill(0.0f); outR.fill(0.0f);
    looper.process(nullptr, nullptr, outL.data(), outR.data(), 4);
    for (int i=0;i<4;++i) if (!near(outL[i],inL[i]) || !near(outR[i],inR[i])) return 7;

    std::array<float,4> addL{0.05f,0.05f,0.05f,0.05f};
    std::array<float,4> addR{0.1f,0.1f,0.1f,0.1f};
    if (!looper.overdub()) return 8;
    looper.process(addL.data(), addR.data(), outL.data(), outR.data(), 4);
    if (!looper.play()) return 9; // complete overdub

    looper.rewind();
    looper.process(nullptr,nullptr,outL.data(),outR.data(),4);
    for(int i=0;i<4;++i) if(!near(outL[i],inL[i]+0.05f)||!near(outR[i],inR[i]+0.1f)) return 10;

    if (!looper.undo()) return 11;
    looper.rewind();
    looper.process(nullptr,nullptr,outL.data(),outR.data(),4);
    for(int i=0;i<4;++i) if(!near(outL[i],inL[i])||!near(outR[i],inR[i])) return 12;

    if (!looper.redo()) return 13;
    looper.rewind();
    looper.process(nullptr,nullptr,outL.data(),outR.data(),4);
    for(int i=0;i<4;++i) if(!near(outL[i],inL[i]+0.05f)||!near(outR[i],inR[i]+0.1f)) return 14;

    if (!looper.stop() || looper.state()!=LooperState::stopped) return 15;
    const auto snap=looper.snapshot();
    if(snap.sampleRate!=10 || snap.left.size()!=4 || snap.right.size()!=4) return 16;
    if(!near(snap.left[0],0.15f)||!near(snap.right[0],0.5f)) return 17;

    const auto wav = std::filesystem::temp_directory_path() / "pmx_looper_export_test.wav";
    std::filesystem::remove(wav);
    const auto exported = pmx::looper::LoopExportService::writeWav24Async(snap, wav).get();
    if (!exported.ok || !std::filesystem::exists(wav) || std::filesystem::file_size(wav) <= 44) return 18;
    std::filesystem::remove(wav);

    looper.clear();
    if(looper.state()!=LooperState::empty || looper.loopFrames()!=0) return 19;

    LooperEngine limit;
    limit.prepare(4.0,1.0); // exactly 4 frames max
    limit.record();
    std::array<float,6> ones{1,1,1,1,1,1}, zeros{};
    std::array<float,6> limitedOutL{}, limitedOutR{};
    limit.process(ones.data(),ones.data(),limitedOutL.data(),limitedOutR.data(),6);
    if(limit.loopFrames()!=4 || limit.state()!=LooperState::playing) return 20;

    // Clearing a loop must discard old overdub samples, including a partial take.
    looper.clear();
    looper.record();
    looper.process(inL.data(), inR.data(), outL.data(), outR.data(), 4);
    looper.play();
    looper.overdub();
    looper.process(addL.data(), addR.data(), outL.data(), outR.data(), 2);
    looper.play();
    looper.rewind();
    looper.process(nullptr, nullptr, outL.data(), outR.data(), 4);
    if (!near(outL[0], 0.15f) || !near(outL[2], 0.3f)) return 21;
    looper.undo();
    looper.rewind();
    looper.process(nullptr, nullptr, outL.data(), outR.data(), 4);
    if (!near(outL[0], 0.1f) || !near(outL[2], 0.3f)) return 22;
    looper.redo();
    looper.overdub();
    looper.process(addL.data(), addR.data(), outL.data(), outR.data(), 1);
    looper.stop();
    const auto partial = looper.snapshot();
    if (!near(partial.left[0], 0.2f) || !near(partial.left[1], 0.25f)) return 23;
    looper.undo();
    const auto undone = looper.snapshot();
    if (!near(undone.left[0], 0.15f) || !near(undone.left[1], 0.25f)) return 24;
    return 0;
}
