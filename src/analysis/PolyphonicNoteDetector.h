#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include "instruments/GuitarProfile.h"

namespace pmx::analysis
{
struct PolyphonicNote
{
    int midiNote { -1 };
    double frequencyHz {};
    float confidence {};
};

struct PolyphonicSnapshot
{
    static constexpr int maxNotes = 6;
    std::array<PolyphonicNote,maxNotes> notes {};
    int noteCount {};
    float frameLevel {};
    std::uint64_t serial {};
};

class PolyphonicNoteDetector final
{
public:
    PolyphonicNoteDetector() noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setProfile(const instruments::GuitarProfile& profile) noexcept;
    void process(const float* samples,int numSamples) noexcept;

    [[nodiscard]] PolyphonicSnapshot snapshot() const noexcept;

private:
    static constexpr int windowSize = 4096;
    static constexpr int analysisHop = 1024;
    static constexpr int maxCandidates = 64;

    struct Score
    {
        int candidateIndex { -1 };
        int midi { -1 };
        double frequency {};
        float amplitude {};
        float supported {};
        bool localPeak {};
        bool selected {};
    };

    [[nodiscard]] float spectralAmplitude(double frequency) const noexcept;
    void rebuildCandidateStateIfNeeded() noexcept;
    void analyse() noexcept;
    [[nodiscard]] bool explainedBySelectedHarmonic(const Score& candidate,
                                                    const std::array<Score,PolyphonicSnapshot::maxNotes>& selected,
                                                    int selectedCount) const noexcept;
    void publish(const std::array<Score,PolyphonicSnapshot::maxNotes>& selected,
                 int selectedCount,
                 float frameLevel) noexcept;
    void publishEmpty(float frameLevel=0.0f) noexcept;

    double sampleRateHz { 44100.0 };
    std::array<float,windowSize> history {};
    std::array<float,windowSize> analysisWindow {};
    double analysisWindowScale { 1.0 };
    int writePosition {};
    int historyCount {};
    int samplesSinceAnalysis {};

    std::array<std::atomic<int>,maxCandidates> candidateMidi {};
    std::array<std::atomic<double>,maxCandidates> candidateFrequency {};
    std::atomic<int> candidateCount { 0 };
    std::atomic<std::uint64_t> profileGeneration { 1 };
    std::uint64_t localProfileGeneration {};
    std::array<unsigned char,maxCandidates> evidence {};

    std::array<std::atomic<int>,PolyphonicSnapshot::maxNotes> publishedMidi {};
    std::array<std::atomic<double>,PolyphonicSnapshot::maxNotes> publishedFrequency {};
    std::array<std::atomic<float>,PolyphonicSnapshot::maxNotes> publishedConfidence {};
    std::atomic<int> publishedCount { 0 };
    std::atomic<float> publishedFrameLevel { 0.0f };
    std::atomic<std::uint64_t> publishedSerial { 0 };
};
} // namespace pmx::analysis
