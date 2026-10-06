#include "ProcessingEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::audio
{
ProcessingEngine::ProcessingEngine() : metronome(tempoService), rhythmDrums(tempoService) {}

void ProcessingEngine::setInputGainDb(float db) noexcept
{
    if(std::isfinite(db)) inputGain.store(std::pow(10.0f, std::clamp(db,-60.0f,24.0f) / 20.0f), std::memory_order_relaxed);
}

void ProcessingEngine::setOutputGainDb(float db) noexcept
{
    if(std::isfinite(db)) outputGain.store(std::pow(10.0f, std::clamp(db,-60.0f,6.0f) / 20.0f), std::memory_order_relaxed);
}

void ProcessingEngine::prepare(double newSampleRate, int newMaxBlockSize, int, int)
{
    preparedSampleRate = newSampleRate;
    preparedBlockSize = newMaxBlockSize;
    signalMetrics.reset();
    guitarRack.prepare(newSampleRate, newMaxBlockSize);
    irProcessor.reset();
    metronome.prepare(newSampleRate);
    rhythmDrums.prepare(newSampleRate);
    stringDrums.prepare(newSampleRate);
    tunerCapture.prepare(32768);
    musicalAnalyzer.prepare(newSampleRate);
    noteSolver.reset();
    polyphonicDetector.prepare(newSampleRate);
    performanceEvents.reset();
    looperEngine.preparePreserving(newSampleRate, 120.0);
    quickRecorder.prepare(newSampleRate, 2, 2.0);
    loopScratchLeft.assign(static_cast<std::size_t>(std::max(1, newMaxBlockSize)), 0.0f);
    loopScratchRight.assign(static_cast<std::size_t>(std::max(1, newMaxBlockSize)), 0.0f);
    metronomeScratch.assign(static_cast<std::size_t>(std::max(1, newMaxBlockSize)), 0.0f);
    rhythmScratch.assign(static_cast<std::size_t>(std::max(1, newMaxBlockSize)), 0.0f);
    stringDrumScratch.assign(static_cast<std::size_t>(std::max(1, newMaxBlockSize)), 0.0f);
    muted.store(true, std::memory_order_relaxed);
    commandRead.store(commandWrite.load());
    publishLoopStatus();
}

void ProcessingEngine::stopped() noexcept
{
    muted.store(true, std::memory_order_relaxed);
    signalMetrics.reset();
    musicalAnalyzer.reset();
    noteSolver.reset();
    polyphonicDetector.reset();
    performanceEvents.reset();
    looperEngine.stop();
    publishLoopStatus();
}

void ProcessingEngine::process(const float* const* inputs, int numInputs,
                               float* const* outputs, int numOutputs,
                               int numSamples) noexcept
{
    if (outputs == nullptr || numOutputs <= 0 || numSamples <= 0) return;
    if (numSamples>preparedBlockSize.load() || preparedBlockSize.load()<=0)
    {
        for(int c=0;c<numOutputs;++c) if(outputs[c]) std::fill_n(outputs[c],numSamples,0.0f);
        return;
    }
    applyLoopCommands();

    float inPeak = 0.0f;
    const bool hasLiveInput = inputs != nullptr && numInputs > 0 && inputs[0] != nullptr;
    if (hasLiveInput)
    {
        tunerCapture.push(inputs[0], numSamples);
        musicalAnalyzer.process(inputs[0], numSamples);
        polyphonicDetector.process(inputs[0], numSamples);
        for (int i = 0; i < numSamples; ++i)
            if (std::isfinite(inputs[0][i])) inPeak = std::max(inPeak, std::abs(inputs[0][i]));
    }
    else
    {
        musicalAnalyzer.process(nullptr, numSamples);
        polyphonicDetector.process(nullptr, numSamples);
    }
    noteSolver.process(musicalAnalyzer.snapshot());
    performanceEvents.process(noteSolver.snapshot(),polyphonicDetector.snapshot());
    signalMetrics.updateInput(inPeak);

    const bool masterMuted = muted.load(std::memory_order_relaxed);
    for (int c = 0; c < numOutputs; ++c)
    {
        auto* out = outputs[c];
        if (out == nullptr) continue;
        if (masterMuted || !hasLiveInput || stringDrums.isEnabled())
            std::fill_n(out, numSamples, 0.0f);
        else
        {
            const auto gain = inputGain.load(std::memory_order_relaxed);
            for (int i = 0; i < numSamples; ++i)
            {
                const auto v = inputs[0][i];
                out[i] = std::isfinite(v) ? v * gain : 0.0f;
            }
        }
    }

    if (!masterMuted)
    {
        if (!fxBypass.load(std::memory_order_relaxed) && !stringDrums.isEnabled() && outputs[0] != nullptr)
        {
            guitarRack.processPreModels(outputs[0], numSamples);
            if (guitarRack.isEnabled(dsp::RackModule::nam) && namProcessor.loaded())
                namProcessor.process(outputs[0], numSamples);
            if (guitarRack.isEnabled(dsp::RackModule::ir) && irProcessor.loaded())
                irProcessor.process(outputs[0], numSamples);
            guitarRack.processPostModels(outputs[0], numSamples);
            for (int c = 1; c < numOutputs; ++c)
                if (outputs[c] != nullptr) std::copy_n(outputs[0], numSamples, outputs[c]);
        }

        if (numSamples <= static_cast<int>(stringDrumScratch.size()))
        {
            std::fill_n(stringDrumScratch.data(), numSamples, 0.0f);
            stringDrums.process(hasLiveInput ? inputs[0] : nullptr, stringDrumScratch.data(), numSamples);
            for (int c = 0; c < numOutputs; ++c)
                if (outputs[c] != nullptr)
                    for (int i = 0; i < numSamples; ++i)
                        outputs[c][i] += stringDrumScratch[static_cast<std::size_t>(i)];
        }

        if (numSamples <= static_cast<int>(loopScratchLeft.size()))
        {
            const auto before=looperEngine.state();
            const auto frame=before==looper::LooperState::recording?looperEngine.recordedFrames():looperEngine.playbackPosition();
            const float* liveL = outputs[0];
            const float* liveR = numOutputs > 1 && outputs[1] != nullptr ? outputs[1] : outputs[0];
            looperEngine.process(liveL, liveR, loopScratchLeft.data(), loopScratchRight.data(), numSamples);
            if(before==looper::LooperState::recording||before==looper::LooperState::playing||before==looper::LooperState::overdubbing)
            {
                const auto length=looperEngine.loopFrames();
                const auto stride=static_cast<std::size_t>(std::max(1.0,preparedSampleRate.load()/10));
                for(int i=0;i<numSamples;++i)
                {
                    auto pos=frame+static_cast<std::size_t>(i);if(before!=looper::LooperState::recording&&length>0)pos%=length;
                    const auto bin=std::min(pos/stride,waveformPeaks.size()-1);
                    const float value=before==looper::LooperState::recording?(liveL?std::abs(liveL[i]):0):std::max(std::abs(loopScratchLeft[static_cast<std::size_t>(i)]),std::abs(loopScratchRight[static_cast<std::size_t>(i)]));
                    if(pos%stride==0)waveformPeaks[bin].store(value,std::memory_order_relaxed);
                    else waveformPeaks[bin].store(std::max(waveformPeaks[bin].load(std::memory_order_relaxed),value),std::memory_order_relaxed);
                }
            }
            if (outputs[0] != nullptr)
                for (int i = 0; i < numSamples; ++i) outputs[0][i] += loopScratchLeft[static_cast<std::size_t>(i)]*loopLevel.load();
            if (numOutputs > 1 && outputs[1] != nullptr)
                for (int i = 0; i < numSamples; ++i) outputs[1][i] += loopScratchRight[static_cast<std::size_t>(i)]*loopLevel.load();
            for (int c = 2; c < numOutputs; ++c)
                if (outputs[c] != nullptr && outputs[0] != nullptr) std::copy_n(outputs[0], numSamples, outputs[c]);
        }

        if (numSamples <= static_cast<int>(metronomeScratch.size()))
        {
            std::fill_n(metronomeScratch.data(), numSamples, 0.0f);
            metronome.process(metronomeScratch.data(), numSamples);
            for (int c = 0; c < numOutputs; ++c)
                if (outputs[c] != nullptr)
                    for (int i = 0; i < numSamples; ++i)
                        outputs[c][i] += metronomeScratch[static_cast<std::size_t>(i)];
        }

        if (numSamples <= static_cast<int>(rhythmScratch.size()))
        {
            std::fill_n(rhythmScratch.data(), numSamples, 0.0f);
            rhythmDrums.process(rhythmScratch.data(), numSamples);
            for (int c = 0; c < numOutputs; ++c)
                if (outputs[c] != nullptr)
                    for (int i = 0; i < numSamples; ++i)
                        outputs[c][i] += rhythmScratch[static_cast<std::size_t>(i)];
        }
    }

    const auto masterGain = outputGain.load(std::memory_order_relaxed);
    for (int c = 0; c < numOutputs; ++c)
        if (outputs[c] != nullptr)
            for (int i = 0; i < numSamples; ++i) outputs[c][i] *= masterGain;

    outputProtector.process(outputs, numOutputs, numSamples);

    if (quickRecorder.isRecording() && outputs[0] != nullptr)
    {
        const float* right = numOutputs > 1 && outputs[1] != nullptr ? outputs[1] : outputs[0];
        quickRecorder.push(outputs[0], right, numSamples);
    }

    float outPeak = 0.0f;
    for (int c = 0; c < numOutputs; ++c)
        if (outputs[c] != nullptr)
            for (int i = 0; i < numSamples; ++i)
                outPeak = std::max(outPeak, std::abs(outputs[c][i]));
    signalMetrics.updateOutput(outPeak);
    publishLoopStatus();
}

bool ProcessingEngine::requestLoopCommand(LoopCommand command) noexcept
{
    const auto write=commandWrite.load(std::memory_order_relaxed);
    const auto next=(write+1)%loopCommands.size();
    if(next==commandRead.load(std::memory_order_acquire)) return false;
    loopCommands[write]=command;
    commandWrite.store(static_cast<unsigned>(next),std::memory_order_release);
    return true;
}

void ProcessingEngine::applyLoopCommands() noexcept
{
    auto read=commandRead.load(std::memory_order_relaxed);
    const auto write=commandWrite.load(std::memory_order_acquire);
    while(read!=write)
    {
        switch(loopCommands[read])
        {
            case LoopCommand::record: if(!looperEngine.record())loopError.store(true);else for(auto& peak:waveformPeaks)peak.store(0,std::memory_order_relaxed);break;
            case LoopCommand::play: if(!looperEngine.play())loopError.store(true);break;
            case LoopCommand::overdub: if(!looperEngine.overdub())loopError.store(true);break;
            case LoopCommand::stop: looperEngine.stop(); break;
            case LoopCommand::undo: looperEngine.undo(); break;
            case LoopCommand::redo: looperEngine.redo(); break;
            case LoopCommand::clear: looperEngine.clear();for(auto& peak:waveformPeaks)peak.store(0,std::memory_order_relaxed);break;
        }
        read=static_cast<unsigned>((read+1)%loopCommands.size());
    }
    commandRead.store(read,std::memory_order_release);
}

void ProcessingEngine::publishLoopStatus() noexcept
{
    publishedRecorded.store(looperEngine.recordedFrames());
    publishedLength.store(looperEngine.loopFrames());
    publishedPosition.store(looperEngine.playbackPosition());
    publishedUndo.store(looperEngine.canUndo()); publishedRedo.store(looperEngine.canRedo());
    publishedLoopState.store(looperEngine.state(),std::memory_order_release);
}

LoopStatus ProcessingEngine::loopStatus() const noexcept
{
    return {publishedLoopState.load(std::memory_order_acquire), publishedRecorded.load(), publishedLength.load(), publishedPosition.load(),
            publishedUndo.load(),publishedRedo.load(),commandRead.load()!=commandWrite.load()};
}

std::array<float,128> ProcessingEngine::loopWaveform() const noexcept
{
    std::array<float,128> result{};const auto frames=publishedRecorded.load();const double sr=preparedSampleRate.load();
    if(frames==0||sr<=0)return result;
    const auto count=std::min(waveformPeaks.size(),static_cast<std::size_t>(std::ceil(frames/(sr/10))));
    for(std::size_t i=0;i<result.size();++i){auto a=i*count/result.size(),b=std::max(a+1,(i+1)*count/result.size());for(auto j=a;j<b&&j<waveformPeaks.size();++j)result[i]=std::max(result[i],waveformPeaks[j].load(std::memory_order_relaxed));}
    return result;
}
} // namespace pmx::audio
