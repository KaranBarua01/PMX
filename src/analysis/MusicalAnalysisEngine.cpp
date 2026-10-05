#include "MusicalAnalysisEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::analysis
{
namespace
{
constexpr double minimumPitchHz = 45.0;
constexpr double maximumPitchHz = 1400.0;
constexpr float onsetFloor = 0.004f;
constexpr float releaseFloor = 0.0015f;
}

void MusicalAnalysisEngine::prepare(double newSampleRate) noexcept
{
    sampleRateHz = std::isfinite(newSampleRate) && newSampleRate > 0.0 ? newSampleRate : 44100.0;
    reset();
}

void MusicalAnalysisEngine::reset() noexcept
{
    history.fill(0.0f);
    writePosition = 0;
    historyCount = 0;
    samplesSincePitchAnalysis = 0;
    fastEnvelope = 0.0f;
    slowEnvelope = 0.0f;
    previousMagnitude = 0.0f;
    previousPitchHz = 0.0;
    localOnsetSerial = 0;
    localReleaseSerial = 0;
    samplesSinceOnset = 1000000;
    quietSamples = 0;
    localNoteActive = false;

    publishedActive.store(false, std::memory_order_relaxed);
    publishedFrequency.store(0.0, std::memory_order_relaxed);
    publishedMidi.store(-1, std::memory_order_relaxed);
    publishedConfidence.store(0.0f, std::memory_order_relaxed);
    publishedAmplitude.store(0.0f, std::memory_order_relaxed);
    publishedAttack.store(0.0f, std::memory_order_relaxed);
    publishedPitchDelta.store(0.0f, std::memory_order_relaxed);
    publishedMotion.store(static_cast<int>(PitchMotion::unknown), std::memory_order_relaxed);
    publishedOnsetSerial.store(0, std::memory_order_relaxed);
    publishedReleaseSerial.store(0, std::memory_order_relaxed);
}

void MusicalAnalysisEngine::process(const float* samples, int numSamples) noexcept
{
    if (numSamples <= 0) return;

    const int releaseHold = std::max(1, static_cast<int>(sampleRateHz * 0.055));
    const int onsetCooldown = std::max(1, static_cast<int>(sampleRateHz * 0.040));

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = samples ? samples[i] : 0.0f;
        if (!std::isfinite(sample)) sample = 0.0f;

        history[static_cast<std::size_t>(writePosition)] = sample;
        writePosition = (writePosition + 1) % historySize;
        historyCount = std::min(historyCount + 1, historySize);

        const float magnitude = std::abs(sample);
        fastEnvelope = fastEnvelope * 0.93f + magnitude * 0.07f;
        slowEnvelope = slowEnvelope * 0.9992f + magnitude * 0.0008f;
        publishedAmplitude.store(fastEnvelope, std::memory_order_relaxed);

        if (samplesSinceOnset < 1000000) ++samplesSinceOnset;

        const float adaptiveThreshold = std::max(onsetFloor, slowEnvelope * 2.6f);
        const bool crossedThreshold = previousMagnitude <= adaptiveThreshold && magnitude > adaptiveThreshold;
        const bool strongRise = magnitude > adaptiveThreshold && magnitude > previousMagnitude * 1.12f;

        if (!localNoteActive && samplesSinceOnset >= onsetCooldown && (crossedThreshold || strongRise))
        {
            localNoteActive = true;
            quietSamples = 0;
            samplesSinceOnset = 0;
            ++localOnsetSerial;
            const float attack = std::clamp((magnitude - slowEnvelope) * 7.0f, 0.0f, 1.0f);
            publishedAttack.store(attack, std::memory_order_relaxed);
            publishedOnsetSerial.store(localOnsetSerial, std::memory_order_release);
        }

        if (localNoteActive)
        {
            if (fastEnvelope < releaseFloor)
            {
                ++quietSamples;
                if (quietSamples >= releaseHold)
                {
                    localNoteActive = false;
                    quietSamples = 0;
                    ++localReleaseSerial;
                    previousPitchHz = 0.0;
                    publishedReleaseSerial.store(localReleaseSerial, std::memory_order_release);
                    publishSilencePitch();
                }
            }
            else
            {
                quietSamples = 0;
            }
        }

        publishedActive.store(localNoteActive, std::memory_order_relaxed);
        previousMagnitude = magnitude;

        ++samplesSincePitchAnalysis;
        if (samplesSincePitchAnalysis >= analysisHop)
        {
            samplesSincePitchAnalysis = 0;
            analysePitch();
        }
    }
}

float MusicalAnalysisEngine::recentSample(int indexFromOldest, int count) const noexcept
{
    const int oldest = (writePosition - count + historySize) % historySize;
    const int position = (oldest + indexFromOldest) % historySize;
    return history[static_cast<std::size_t>(position)];
}

float MusicalAnalysisEngine::correlationForLag(int lag, int count, int stride) const noexcept
{
    if (lag <= 0 || lag >= count || stride <= 0) return -1.0f;

    double cross = 0.0;
    double leftEnergy = 0.0;
    double rightEnergy = 0.0;
    int pairs = 0;

    for (int i = 0; i + lag < count; i += stride)
    {
        const double a = recentSample(i, count);
        const double b = recentSample(i + lag, count);
        cross += a * b;
        leftEnergy += a * a;
        rightEnergy += b * b;
        ++pairs;
    }

    if (pairs < 16) return -1.0f;
    const double denominator = std::sqrt(leftEnergy * rightEnergy);
    if (denominator < 1.0e-10) return -1.0f;
    return static_cast<float>(cross / denominator);
}

void MusicalAnalysisEngine::analysePitch() noexcept
{
    if (!localNoteActive || historyCount < minimumPitchSamples)
    {
        if (!localNoteActive) publishSilencePitch();
        return;
    }

    const int count = historyCount;
    const int minLag = std::max(2, static_cast<int>(sampleRateHz / maximumPitchHz));
    const int maxLag = std::min(count / 2 - 2, static_cast<int>(sampleRateHz / minimumPitchHz));
    if (maxLag <= minLag + 4) return;

    float globalBest = -1.0f;
    int coarseBestLag = 0;

    for (int lag = minLag; lag <= maxLag; lag += 4)
    {
        const float correlation = correlationForLag(lag, count, 4);
        if (correlation > globalBest)
        {
            globalBest = correlation;
            coarseBestLag = lag;
        }
    }

    if (coarseBestLag == 0 || globalBest < 0.35f)
    {
        publishedConfidence.store(0.0f, std::memory_order_relaxed);
        return;
    }

    // If several periodic peaks are almost equally strong, prefer the shortest
    // period. This avoids interpreting a fundamental as one of its lower
    // subharmonics while still leaving octave/harmonic decisions to the
    // fretboard-aware solver in the next stage.
    const float nearBest = globalBest - 0.035f;
    for (int lag = minLag; lag < coarseBestLag; lag += 4)
    {
        if (correlationForLag(lag, count, 4) >= nearBest)
        {
            coarseBestLag = lag;
            break;
        }
    }

    float refinedBest = -1.0f;
    int refinedLag = coarseBestLag;
    for (int lag = std::max(minLag, coarseBestLag - 6); lag <= std::min(maxLag, coarseBestLag + 6); ++lag)
    {
        const float correlation = correlationForLag(lag, count, 1);
        if (correlation > refinedBest)
        {
            refinedBest = correlation;
            refinedLag = lag;
        }
    }

    if (refinedBest < 0.35f || refinedLag <= 0)
    {
        publishedConfidence.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const double frequency = sampleRateHz / static_cast<double>(refinedLag);
    if (!std::isfinite(frequency) || frequency < minimumPitchHz || frequency > maximumPitchHz)
    {
        publishedConfidence.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float levelConfidence = std::clamp((fastEnvelope - 0.0015f) / 0.035f, 0.0f, 1.0f);
    const float pitchConfidence = std::clamp((refinedBest - 0.35f) / 0.60f, 0.0f, 1.0f);
    const float confidence = pitchConfidence * (0.30f + 0.70f * levelConfidence);
    publishPitch(frequency, confidence);
}

void MusicalAnalysisEngine::publishPitch(double frequency, float confidence) noexcept
{
    float deltaCents = 0.0f;
    PitchMotion motion = PitchMotion::unknown;

    if (previousPitchHz > 0.0)
    {
        deltaCents = static_cast<float>(1200.0 * std::log2(frequency / previousPitchHz));
        if (std::abs(deltaCents) < 7.0f) motion = PitchMotion::stable;
        else motion = deltaCents > 0.0f ? PitchMotion::rising : PitchMotion::falling;
    }

    const double midiExact = 69.0 + 12.0 * std::log2(frequency / 440.0);
    const int midi = static_cast<int>(std::lround(midiExact));

    previousPitchHz = frequency;
    publishedFrequency.store(frequency, std::memory_order_relaxed);
    publishedMidi.store(midi, std::memory_order_relaxed);
    publishedConfidence.store(std::clamp(confidence, 0.0f, 1.0f), std::memory_order_relaxed);
    publishedPitchDelta.store(deltaCents, std::memory_order_relaxed);
    publishedMotion.store(static_cast<int>(motion), std::memory_order_relaxed);
}

void MusicalAnalysisEngine::publishSilencePitch() noexcept
{
    publishedFrequency.store(0.0, std::memory_order_relaxed);
    publishedMidi.store(-1, std::memory_order_relaxed);
    publishedConfidence.store(0.0f, std::memory_order_relaxed);
    publishedPitchDelta.store(0.0f, std::memory_order_relaxed);
    publishedMotion.store(static_cast<int>(PitchMotion::unknown), std::memory_order_relaxed);
}

MusicalAnalysisSnapshot MusicalAnalysisEngine::snapshot() const noexcept
{
    MusicalAnalysisSnapshot result;
    result.noteActive = publishedActive.load(std::memory_order_relaxed);
    result.frequencyHz = publishedFrequency.load(std::memory_order_relaxed);
    result.midiNote = publishedMidi.load(std::memory_order_relaxed);
    result.confidence = publishedConfidence.load(std::memory_order_relaxed);
    result.amplitude = publishedAmplitude.load(std::memory_order_relaxed);
    result.attack = publishedAttack.load(std::memory_order_relaxed);
    result.pitchDeltaCents = publishedPitchDelta.load(std::memory_order_relaxed);
    result.motion = static_cast<PitchMotion>(publishedMotion.load(std::memory_order_relaxed));
    result.onsetSerial = publishedOnsetSerial.load(std::memory_order_acquire);
    result.releaseSerial = publishedReleaseSerial.load(std::memory_order_acquire);
    return result;
}
} // namespace pmx::analysis
