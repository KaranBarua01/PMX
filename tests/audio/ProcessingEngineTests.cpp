#include <cmath>
#include <limits>
#include <filesystem>
#include <cstdlib>
#include <new>
#include "audio/ProcessingEngine.h"

namespace {thread_local bool guardAllocations=false;thread_local unsigned callbackAllocations=0;}
void* operator new(std::size_t count){if(guardAllocations)++callbackAllocations;if(auto* p=std::malloc(count?count:1))return p;throw std::bad_alloc{};}
void* operator new[](std::size_t count){return ::operator new(count);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

int main()
{
    pmx::audio::ProcessingEngine engine;
    engine.prepare(44100.0, 128, 1, 2);

    float in[] {0.25f, -0.5f, std::numeric_limits<float>::quiet_NaN(), 2.0f};
    const float* inputs[] {in};
    float l[4] {}, r[4] {};
    float* outputs[] {l, r};

    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.process(inputs, 1, outputs, 2, 4);

    if (l[0] != 0.25f || r[1] != -0.5f) return 1;
    if (! std::isfinite(l[2]) || l[2] != 0.0f) return 2;
    if (std::abs(l[3]) > 1.0f || std::abs(r[3]) > 1.0f) return 3;
    auto m = engine.metrics();
    if (m.inputPeak < 1.9f) return 4;
    if (m.outputPeak <= 0.0f || m.outputPeak > 1.0f) return 5;

    engine.setMuted(true);
    engine.process(inputs, 1, outputs, 2, 4);
    for (float v : l) if (v != 0.0f) return 6;
    for (float v : r) if (v != 0.0f) return 7;

    // Looper captures the processed live signal and rejoins after the rack.
    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.looper().clear();
    if (!engine.looper().record()) return 8;
    float take[] {0.1f, 0.2f, 0.3f, 0.4f};
    const float* takeInputs[] {take};
    engine.process(takeInputs, 1, outputs, 2, 4);
    if (!engine.looper().play()) return 9;
    float silenceIn[] {0,0,0,0};
    const float* silenceInputs[] {silenceIn};
    engine.process(silenceInputs, 1, outputs, 2, 4);
    if (std::abs(l[0]-0.1f)>0.0001f || std::abs(r[3]-0.4f)>0.0001f) return 10;

    // Quick recorder receives the protected master output without blocking the callback.
    const auto recPath = std::filesystem::temp_directory_path() / "pmx_processing_engine_take.wav";
    std::filesystem::remove(recPath);
    if (!engine.recorder().start(recPath).ok) return 11;
    engine.process(takeInputs, 1, outputs, 2, 4);
    const auto recResult = engine.recorder().stop();
    if (!recResult.ok || engine.recorder().framesAccepted() != 4) return 12;
    std::filesystem::remove(recPath);

    // IR is inserted between drive and post-cab effects and is skipped by FX bypass.
    engine.looper().clear();
    auto irData = std::make_shared<pmx::ir::IrData>();
    irData->sampleRate = 44100.0;
    irData->taps = { 0.5f };
    engine.ir().setPrepared(irData);
    engine.rack().setEnabled(pmx::dsp::RackModule::ir, true);
    engine.setFxBypass(false);
    float irIn[] {0.4f, -0.2f, 0.0f, 0.1f};
    const float* irInputs[] {irIn};
    engine.process(irInputs, 1, outputs, 2, 4);
    if (std::abs(l[0] - 0.2f) > 0.0001f || std::abs(l[1] + 0.1f) > 0.0001f) return 13;

    engine.setFxBypass(true);
    engine.process(irInputs, 1, outputs, 2, 4);
    if (std::abs(l[0] - 0.4f) > 0.0001f) return 14;

    // The metronome joins after the looper/live rack and follows the shared tempo.
    engine.looper().clear();
    engine.setFxBypass(true);
    engine.tempo().setBpm(300.0);
    engine.setMetronomeEnabled(true);
    engine.prepare(1000.0, 128, 1, 2);
    engine.setMuted(false);
    engine.tempo().setBpm(300.0);
    engine.setMetronomeEnabled(true);
    float metIn[128] {};
    const float* metInputs[] {metIn};
    float metL[128] {}, metR[128] {};
    float* metOutputs[] {metL, metR};
    engine.process(metInputs, 1, metOutputs, 2, 128);
    engine.process(metInputs, 1, metOutputs, 2, 128);
    bool heardClick = false;
    for (float v : metL) if (std::abs(v) > 0.0001f) heardClick = true;
    if (!heardClick) return 15;

    // Settings input/output gain controls must affect the live path using dB units.
    engine.setMetronomeEnabled(false);
    engine.looper().clear();
    engine.prepare(44100.0, 128, 1, 2);
    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.setInputGainDb(-6.0206f);
    engine.setOutputGainDb(0.0f);
    float gainIn[] {0.4f};
    const float* gainInputs[] {gainIn};
    float gainL[] {0.0f}, gainR[] {0.0f};
    float* gainOutputs[] {gainL, gainR};
    engine.process(gainInputs, 1, gainOutputs, 2, 1);
    if (std::abs(gainL[0] - 0.2f) > 0.002f) return 16;
    engine.setInputGainDb(0.0f);
    engine.setOutputGainDb(-6.0206f);
    engine.process(gainInputs, 1, gainOutputs, 2, 1);
    if (std::abs(gainL[0] - 0.2f) > 0.002f) return 17;
    engine.setInputGainDb(std::numeric_limits<float>::quiet_NaN());
    engine.process(gainInputs, 1, gainOutputs, 2, 1);
    if (std::abs(gainL[0]-0.2f)>0.002f) return 18;
    // Unexpected oversized blocks must mute safely rather than overrun prepared storage.
    std::vector<float> oversized(129,0.5f), oversizedOutput(129,1.0f);
    const float* oversizedInputs[]{oversized.data()};
    float* oversizedOutputs[]{oversizedOutput.data()};
    engine.process(oversizedInputs,1,oversizedOutputs,1,129);
    for(float v:oversizedOutput) if(v!=0) return 19;
    engine.prepare(44100,128,1,2);
    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.setInputGainDb(0);
    engine.setOutputGainDb(0);
    if(!engine.requestLoopCommand(pmx::audio::LoopCommand::record)) return 20;
    if(engine.loopStatus().state != pmx::looper::LooperState::empty) return 21;
    engine.process(takeInputs,1,outputs,2,4);
    if(engine.loopStatus().recordedFrames != 4) return 22;
    const auto waveform=engine.loopWaveform();
    if(*std::max_element(waveform.begin(),waveform.end())<=0)return 29;
    engine.requestLoopCommand(pmx::audio::LoopCommand::stop);
    engine.process(silenceInputs,1,outputs,2,4);
    if(engine.loopStatus().state != pmx::looper::LooperState::stopped) return 23;
    engine.prepare(44100,128,1,2);
    if(engine.loopStatus().loopFrames!=4)return 27;
    engine.prepare(48000,128,1,2);
    if(engine.loopStatus().loopFrames<4)return 28;
    for(int i=0;i<31;++i) if(!engine.requestLoopCommand(pmx::audio::LoopCommand::clear)) return 24;
    if(engine.requestLoopCommand(pmx::audio::LoopCommand::clear)) return 25;
    engine.process(silenceInputs,1,outputs,2,4);
    if(engine.loopStatus().state != pmx::looper::LooperState::empty) return 26;
    engine.setMuted(false);engine.setFxBypass(false);
    for(unsigned i=0;i<9;++i)engine.rack().setEnabled(static_cast<pmx::dsp::RackModule>(i),true);
    engine.requestLoopCommand(pmx::audio::LoopCommand::record);
    guardAllocations=true;
    for(int i=0;i<100;++i)engine.process(takeInputs,1,outputs,2,4);
    engine.requestLoopCommand(pmx::audio::LoopCommand::play);
    engine.process(takeInputs,1,outputs,2,4);
    engine.requestLoopCommand(pmx::audio::LoopCommand::overdub);
    engine.process(takeInputs,1,outputs,2,4);
    engine.requestLoopCommand(pmx::audio::LoopCommand::clear);
    engine.process(takeInputs,1,outputs,2,4);
    guardAllocations=false;
    if(callbackAllocations!=0)return 30;

    // Rhythm drums join the protected master path, follow shared BPM, and work with silent guitar input.
    engine.prepare(1000.0,128,1,2);
    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.setMetronomeEnabled(false);
    engine.setRhythmPattern(1);
    engine.setRhythmLevel(0.5f);
    engine.setRhythmEnabled(true);
    float drumIn[128]{},drumL[128]{},drumR[128]{};
    const float* drumInputs[]{drumIn};float* drumOutputs[]{drumL,drumR};
    engine.process(drumInputs,1,drumOutputs,2,128);
    bool heardDrums=false;for(float v:drumL)if(std::abs(v)>0.0001f)heardDrums=true;
    if(!heardDrums)return 31;
    engine.setRhythmEnabled(false);

    // String drum mode replaces the dry guitar hit and emits the mapped drum after pitch classification.
    engine.prepare(44100.0,128,1,2);
    engine.setMuted(false);
    engine.setFxBypass(true);
    engine.setStringDrumsLevel(0.8f);
    engine.setStringDrumsEnabled(true);
    std::array<float,1536> padInput{};
    for(std::size_t i=0;i<padInput.size();++i)
    {
        const auto time=static_cast<double>(i)/44100.0;
        padInput[i]=static_cast<float>(0.38*std::exp(-time*6.0)*(std::sin(2.0*3.141592653589793*82.4069*time)+0.22*std::sin(4.0*3.141592653589793*82.4069*time)));
    }
    bool heardTriggeredKick=false;
    for(std::size_t offset=0;offset<padInput.size();offset+=128)
    {
        const float* padInputs[]{padInput.data()+offset};
        float padL[128]{},padR[128]{};float* padOutputs[]{padL,padR};
        engine.process(padInputs,1,padOutputs,2,128);
        if(offset==0) for(float v:padL) if(std::abs(v)>0.000001f) return 32;
        for(float v:padL) if(std::abs(v)>0.0001f) heardTriggeredKick=true;
    }
    if(engine.lastStringDrum()!=0)return 33;
    if(!heardTriggeredKick)return 34;
    engine.setStringDrumsEnabled(false);

    // Musical analysis always sees the raw input before mute, gain and guitar effects.
    engine.prepare(44100.0,128,1,2);
    engine.setMuted(true);
    engine.setFxBypass(false);
    std::array<float,128> analysisInput{};
    std::uint64_t analysisSample=0;
    for(int blockIndex=0;blockIndex<40;++blockIndex)
    {
        for(std::size_t i=0;i<analysisInput.size();++i)
        {
            const auto t=static_cast<double>(analysisSample+i)/44100.0;
            analysisInput[i]=static_cast<float>(0.12*std::sin(2.0*3.141592653589793*110.0*t));
        }
        analysisSample+=analysisInput.size();
        const float* analysisInputs[]{analysisInput.data()};
        float analysisL[128]{},analysisR[128]{};
        float* analysisOutputs[]{analysisL,analysisR};
        engine.process(analysisInputs,1,analysisOutputs,2,128);
    }
    const auto musical=engine.musicalAnalysis();
    if(!musical.noteActive)return 35;
    if(musical.frequencyHz<107.0||musical.frequencyHz>113.0)return 36;
    if(musical.midiNote!=45)return 37;
    const auto resolved=engine.resolvedNote();
    if(!resolved.noteActive)return 38;
    if(resolved.midiNote!=45)return 39;
    if(std::abs(resolved.targetFrequencyHz-110.0)>0.2)return 40;
    const auto poly=engine.polyphonicNotes();
    if(poly.noteCount!=1)return 41;
    if(poly.notes[0].midiNote!=45)return 42;

    bool sawResolvedNoteOn=false;
    bool sawPolyphonicNoteOn=false;
    bool sawChord=false;
    pmx::performance::PerformanceEvent performanceEvent;
    while(engine.popPerformanceEvent(performanceEvent))
    {
        if(performanceEvent.type==pmx::performance::PerformanceEventType::noteOn &&
           performanceEvent.source==pmx::performance::PerformanceEventSource::resolved &&
           performanceEvent.midiNote==45)
            sawResolvedNoteOn=true;
        if(performanceEvent.type==pmx::performance::PerformanceEventType::noteOn &&
           performanceEvent.source==pmx::performance::PerformanceEventSource::polyphonic &&
           performanceEvent.midiNote==45)
            sawPolyphonicNoteOn=true;
        if(performanceEvent.type==pmx::performance::PerformanceEventType::chordChanged &&
           performanceEvent.noteCount==1 &&
           performanceEvent.notes[0]==45)
            sawChord=true;
    }
    if(!sawResolvedNoteOn)return 43;
    if(!sawPolyphonicNoteOn)return 44;
    if(!sawChord)return 45;
    if(engine.droppedPerformanceEvents()!=0)return 46;

    // SPECIAL instrument routing is exclusive: selecting one engine must disable
    // every other guitar-triggered instrument instead of stacking sounds.
    engine.setInstrumentMode(pmx::audio::InstrumentMode::synth);
    if(engine.instrumentMode()!=pmx::audio::InstrumentMode::synth)return 47;
    if(!engine.synthEnabled()||engine.bassEnabled()||engine.pianoEnabled()||engine.violinEnabled()||engine.stringDrumsEnabled())return 48;

    engine.setInstrumentMode(pmx::audio::InstrumentMode::piano);
    if(engine.instrumentMode()!=pmx::audio::InstrumentMode::piano)return 49;
    if(engine.synthEnabled()||engine.bassEnabled()||!engine.pianoEnabled()||engine.violinEnabled()||engine.stringDrumsEnabled())return 50;

    engine.setInstrumentMode(pmx::audio::InstrumentMode::drums);
    if(engine.instrumentMode()!=pmx::audio::InstrumentMode::drums)return 51;
    if(engine.synthEnabled()||engine.bassEnabled()||engine.pianoEnabled()||engine.violinEnabled()||!engine.stringDrumsEnabled())return 52;

    engine.setInstrumentMode(pmx::audio::InstrumentMode::none);
    if(engine.instrumentMode()!=pmx::audio::InstrumentMode::none)return 53;
    if(engine.synthEnabled()||engine.bassEnabled()||engine.pianoEnabled()||engine.violinEnabled()||engine.stringDrumsEnabled())return 54;
    return 0;
}
