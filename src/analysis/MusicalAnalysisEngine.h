#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace pmx::analysis
{
enum class PitchMotion : int
{
    unknown = 0,
    stable,
    rising,
    falling
};

struct MusicalAnalysisSnapshot
{
    bool noteActive {};
    double frequencyHz {};
    int midiNote { -1 };
    float confidence {};
    float amplitude {};
    float attack {};
    float pitchDeltaCents {};
    PitchMotion motion { PitchMotion::unknown };
    std::uint64_t onsetSerial {};
    std::uint64_t releaseSerial {};
    std::uint64_t pitchSerial {};
};

class MusicalAnalysisEngine final
{
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(const float* samples, int numSamples) noexcept;

    [[nodiscard]] MusicalAnalysisSnapshot snapshot() const noexcept;

private:
    static constexpr int historySize = 2048;
    static constexpr int minimumPitchSamples = 1536;
    static constexpr int analysisHop = 256;

    [[nodiscard]] float recentSample(int indexFromOldest, int count) const noexcept;
    [[nodiscard]] float correlationForLag(int lag, int count, int stride) const noexcept;
    void analysePitch() noexcept;
    void publishPitch(double frequency, float confidence) noexcept;
    void publishSilencePitch() noexcept;

    double sampleRateHz { 44100.0 };
    std::array<float, historySize> history {};
    int writePosition {};
    int historyCount {};
    int samplesSincePitchAnalysis {};

    float fastEnvelope {};
    float slowEnvelope {};
    float previousMagnitude {};
    double previousPitchHz {};
    std::uint64_t localOnsetSerial {};
    std::uint64_t localReleaseSerial {};
    std::uint64_t localPitchSerial {};
    int samplesSinceOnset { 1000000 };
    int quietSamples {};
    bool localNoteActive {};

    std::atomic<bool> publishedActive { false };
    std::atomic<double> publishedFrequency { 0.0 };
    std::atomic<int> publishedMidi { -1 };
    std::atomic<float> publishedConfidence { 0.0f };
    std::atomic<float> publishedAmplitude { 0.0f };
    std::atomic<float> publishedAttack { 0.0f };
    std::atomic<float> publishedPitchDelta { 0.0f };
    std::atomic<int> publishedMotion { static_cast<int>(PitchMotion::unknown) };
    std::atomic<std::uint64_t> publishedOnsetSerial { 0 };
    std::atomic<std::uint64_t> publishedReleaseSerial { 0 };
    std::atomic<std::uint64_t> publishedPitchSerial { 0 };
};
} // namespace pmx::analysis
