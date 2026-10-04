#include "RhythmDrumMachine.h"
#include <cmath>

namespace pmx::dsp
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr std::array<std::uint16_t, RhythmDrumMachine::patternCount> kickMasks {
    0x0101u, 0x4141u, 0x0581u, 0x4549u
};
constexpr std::array<std::uint16_t, RhythmDrumMachine::patternCount> snareMasks {
    0x1010u, 0x1010u, 0x1010u, 0x1010u
};
constexpr std::array<std::uint16_t, RhythmDrumMachine::patternCount> hatMasks {
    0x5555u, 0x5555u, 0xffffu, 0xffffu
};
}

void RhythmDrumMachine::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    stepPhase = 0.0;
    kickPhase = 0.0;
    snarePhase = 0.0;
    kickEnvelope = snareEnvelope = hatEnvelope = 0.0f;
    previousNoise = 0.0f;
    currentStep = 0;
    wasEnabled = false;
}

float RhythmDrumMachine::noise() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return static_cast<float>((randomState & 0x00ffffffu) / 8388607.5 - 1.0);
}

void RhythmDrumMachine::triggerStep(int step) noexcept
{
    const auto selected = pattern.load(std::memory_order_relaxed);
    const auto bit = static_cast<std::uint16_t>(1u << (step & 15));
    if ((kickMasks[static_cast<std::size_t>(selected)] & bit) != 0)
    {
        kickEnvelope = 1.0f;
        kickPhase = 0.0;
    }
    if ((snareMasks[static_cast<std::size_t>(selected)] & bit) != 0)
    {
        snareEnvelope = 1.0f;
        snarePhase = 0.0;
    }
    if ((hatMasks[static_cast<std::size_t>(selected)] & bit) != 0)
        hatEnvelope = (step % 4 == 0) ? 0.72f : 0.48f;
}

void RhythmDrumMachine::process(float* mono, int numSamples) noexcept
{
    if (!mono || numSamples <= 0) return;

    if (!enabled.load(std::memory_order_relaxed))
    {
        wasEnabled = false;
        kickEnvelope = snareEnvelope = hatEnvelope = 0.0f;
        return;
    }

    if (!wasEnabled)
    {
        stepPhase = 0.0;
        currentStep = 0;
        triggerStep(currentStep);
        wasEnabled = true;
    }

    const auto stepSamples = std::max(1.0, tempo.samplesPerBeat(sampleRate) * 0.25);
    const auto kickDecay = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.115)));
    const auto snareDecay = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.095)));
    const auto hatDecay = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.032)));
    const auto outputLevel = level.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        while (stepPhase >= stepSamples)
        {
            stepPhase -= stepSamples;
            currentStep = (currentStep + 1) & 15;
            triggerStep(currentStep);
        }

        const auto random = noise();

        const double kickHz = 48.0 + 92.0 * static_cast<double>(kickEnvelope);
        kickPhase += 2.0 * pi * kickHz / sampleRate;
        if (kickPhase > 2.0 * pi) kickPhase -= 2.0 * pi;
        const float kick = static_cast<float>(std::sin(kickPhase)) * kickEnvelope;
        kickEnvelope *= kickDecay;

        snarePhase += 2.0 * pi * 185.0 / sampleRate;
        if (snarePhase > 2.0 * pi) snarePhase -= 2.0 * pi;
        const float snare = (random * 0.82f + static_cast<float>(std::sin(snarePhase)) * 0.18f) * snareEnvelope;
        snareEnvelope *= snareDecay;

        const float highPassedNoise = random - previousNoise * 0.92f;
        previousNoise = random;
        const float hat = highPassedNoise * hatEnvelope;
        hatEnvelope *= hatDecay;

        mono[i] += outputLevel * (kick * 0.78f + snare * 0.42f + hat * 0.24f);
        stepPhase += 1.0;
    }
}
} // namespace pmx::dsp
