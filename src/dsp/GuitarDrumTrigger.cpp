#include "GuitarDrumTrigger.h"
#include <cmath>

namespace pmx::dsp
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr std::array<double, 6> openStringHz { 82.4069, 110.0, 146.832, 195.998, 246.942, 329.628 };
}

void GuitarDrumTrigger::prepare(double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    capture.fill(0.0f);
    captureCount = 0;
    cooldownSamples = 0;
    slowEnvelope = 0.0f;
    previousAbs = 0.0f;
    wasEnabled = false;
    kickEnvelope = snareEnvelope = closedHatEnvelope = openHatEnvelope = tomEnvelope = crashEnvelope = 0.0f;
    kickPhase = snarePhase = tomPhase = crashPhase = 0.0;
    previousNoise = 0.0f;
    lastDetected.store(-1, std::memory_order_relaxed);
}

void GuitarDrumTrigger::setEnabled(bool value) noexcept
{
    enabled.store(value, std::memory_order_release);
    lastDetected.store(-1, std::memory_order_relaxed);
}

float GuitarDrumTrigger::noise() noexcept
{
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return static_cast<float>((randomState & 0x00ffffffu) / 8388607.5 - 1.0);
}

float GuitarDrumTrigger::correlationAtLag(int lag) const noexcept
{
    if (lag < 1 || lag >= captureSize - 96) return -1.0f;
    double cross = 0.0, leftEnergy = 0.0, rightEnergy = 0.0;
    constexpr int start = 64;
    for (int i = start; i + lag < captureSize; ++i)
    {
        const auto a = static_cast<double>(capture[static_cast<std::size_t>(i)]);
        const auto b = static_cast<double>(capture[static_cast<std::size_t>(i + lag)]);
        cross += a * b;
        leftEnergy += a * a;
        rightEnergy += b * b;
    }
    const auto denominator = std::sqrt(leftEnergy * rightEnergy);
    if (denominator < 1.0e-10) return -1.0f;
    return static_cast<float>(cross / denominator);
}

GuitarDrumPad GuitarDrumTrigger::classifyCapturedPitch() const noexcept
{
    std::array<float, openStringHz.size()> scores {};
    float globalBest = -2.0f;

    for (int stringIndex = 0; stringIndex < static_cast<int>(openStringHz.size()); ++stringIndex)
    {
        const auto targetLag = sampleRate / openStringHz[static_cast<std::size_t>(stringIndex)];
        const int spread = std::max(2, static_cast<int>(std::lround(targetLag * 0.025)));
        float stringBest = -2.0f;
        for (int lag = static_cast<int>(std::lround(targetLag)) - spread;
             lag <= static_cast<int>(std::lround(targetLag)) + spread; ++lag)
            stringBest = std::max(stringBest, correlationAtLag(lag));
        scores[static_cast<std::size_t>(stringIndex)] = stringBest;
        globalBest = std::max(globalBest, stringBest);
    }

    if (globalBest < 0.30f) return GuitarDrumPad::none;

    // Higher strings naturally correlate at integer multiples of their period
    // (for example B3 can also correlate near the low-E period). Among nearly
    // equal candidates, prefer the shortest period/highest string to avoid
    // octave/subharmonic misclassification.
    constexpr float harmonicTolerance = 0.035f;
    for (int stringIndex = static_cast<int>(openStringHz.size()) - 1; stringIndex >= 0; --stringIndex)
        if (scores[static_cast<std::size_t>(stringIndex)] >= globalBest - harmonicTolerance)
            return static_cast<GuitarDrumPad>(stringIndex);

    return GuitarDrumPad::none;
}

void GuitarDrumTrigger::trigger(GuitarDrumPad pad) noexcept
{
    lastDetected.store(static_cast<int>(pad), std::memory_order_relaxed);
    switch (pad)
    {
        case GuitarDrumPad::lowE: kickEnvelope = 1.0f; kickPhase = 0.0; break;
        case GuitarDrumPad::a: snareEnvelope = 1.0f; snarePhase = 0.0; break;
        case GuitarDrumPad::d: closedHatEnvelope = 1.0f; break;
        case GuitarDrumPad::g: openHatEnvelope = 1.0f; break;
        case GuitarDrumPad::b: tomEnvelope = 1.0f; tomPhase = 0.0; break;
        case GuitarDrumPad::highE: crashEnvelope = 1.0f; crashPhase = 0.0; break;
        case GuitarDrumPad::none: break;
    }
}

float GuitarDrumTrigger::renderVoice() noexcept
{
    const auto random = noise();

    const double kickHz = 48.0 + 88.0 * static_cast<double>(kickEnvelope);
    kickPhase += 2.0 * pi * kickHz / sampleRate;
    if (kickPhase > 2.0 * pi) kickPhase -= 2.0 * pi;
    const float kick = static_cast<float>(std::sin(kickPhase)) * kickEnvelope;
    kickEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.12)));

    snarePhase += 2.0 * pi * 185.0 / sampleRate;
    if (snarePhase > 2.0 * pi) snarePhase -= 2.0 * pi;
    const float snare = (random * 0.82f + static_cast<float>(std::sin(snarePhase)) * 0.18f) * snareEnvelope;
    snareEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.095)));

    const float highNoise = random - previousNoise * 0.94f;
    previousNoise = random;
    const float closedHat = highNoise * closedHatEnvelope;
    const float openHat = highNoise * openHatEnvelope;
    closedHatEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.032)));
    openHatEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.24)));

    tomPhase += 2.0 * pi * (122.0 + 55.0 * static_cast<double>(tomEnvelope)) / sampleRate;
    if (tomPhase > 2.0 * pi) tomPhase -= 2.0 * pi;
    const float tom = static_cast<float>(std::sin(tomPhase)) * tomEnvelope;
    tomEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.18)));

    crashPhase += 2.0 * pi * 5100.0 / sampleRate;
    if (crashPhase > 2.0 * pi) crashPhase -= 2.0 * pi;
    const float crash = (highNoise * 0.72f + static_cast<float>(std::sin(crashPhase)) * 0.12f) * crashEnvelope;
    crashEnvelope *= static_cast<float>(std::exp(-1.0 / (sampleRate * 0.72)));

    return kick * 0.86f + snare * 0.46f + closedHat * 0.24f + openHat * 0.20f + tom * 0.50f + crash * 0.20f;
}

void GuitarDrumTrigger::process(const float* input, float* monoOut, int numSamples) noexcept
{
    if (!monoOut || numSamples <= 0) return;
    const bool active = enabled.load(std::memory_order_acquire);
    if (active != wasEnabled)
    {
        captureCount = 0;
        cooldownSamples = 0;
        slowEnvelope = 0.0f;
        previousAbs = 0.0f;
        if (!active)
            kickEnvelope = snareEnvelope = closedHatEnvelope = openHatEnvelope = tomEnvelope = crashEnvelope = 0.0f;
        wasEnabled = active;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (active && input)
        {
            const float value = std::isfinite(input[i]) ? input[i] : 0.0f;
            const float magnitude = std::abs(value);
            slowEnvelope = slowEnvelope * 0.9992f + magnitude * 0.0008f;
            if (cooldownSamples > 0) --cooldownSamples;

            if (captureCount == 0)
            {
                const float threshold = std::max(0.004f, slowEnvelope * 2.4f);
                const bool crossedThreshold = previousAbs <= threshold && magnitude > threshold;
                const bool fastRise = magnitude > threshold && magnitude > previousAbs * 1.08f;
                const bool onset = cooldownSamples == 0 && (crossedThreshold || fastRise);
                if (onset)
                    captureCount = 1, capture[0] = value;
            }
            else
            {
                if (captureCount < captureSize)
                    capture[static_cast<std::size_t>(captureCount++)] = value;
                if (captureCount >= captureSize)
                {
                    const auto pad = classifyCapturedPitch();
                    if (pad != GuitarDrumPad::none) trigger(pad);
                    captureCount = 0;
                    cooldownSamples = static_cast<int>(sampleRate * 0.075);
                }
            }
            previousAbs = magnitude;
        }
        else
        {
            captureCount = 0;
            previousAbs = 0.0f;
            slowEnvelope *= 0.995f;
        }

        monoOut[i] += level.load(std::memory_order_relaxed) * renderVoice();
    }
}
} // namespace pmx::dsp
