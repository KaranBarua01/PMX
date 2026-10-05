#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include "analysis/MusicalAnalysisEngine.h"
#include "instruments/GuitarProfile.h"

namespace pmx::analysis
{
struct ResolvedNoteSnapshot
{
    bool noteActive {};
    int midiNote { -1 };
    double targetFrequencyHz {};
    double rawFrequencyHz {};
    float centsFromTarget {};
    float confidence {};
    bool transitioning {};
    std::uint64_t onsetSerial {};
    std::uint64_t releaseSerial {};
};

class FretboardNoteSolver final
{
public:
    FretboardNoteSolver() noexcept;

    void reset() noexcept;
    void setProfile(const instruments::GuitarProfile& profile) noexcept;
    void process(const MusicalAnalysisSnapshot& input) noexcept;

    [[nodiscard]] ResolvedNoteSnapshot snapshot() const noexcept;

private:
    static constexpr int maxCandidates =
        static_cast<int>(instruments::GuitarProfile::stringCount) * (instruments::GuitarProfile::maxFret + 1);

    struct Candidate
    {
        double frequency {};
        int midi { -1 };
        float cents {};
        bool valid {};
    };

    [[nodiscard]] Candidate nearestCandidate(double frequency) const noexcept;
    [[nodiscard]] static float centsBetween(double frequency, double target) noexcept;
    void publish(bool active,
                 int midi,
                 double target,
                 double raw,
                 float cents,
                 float confidence,
                 bool transitioning,
                 std::uint64_t onset,
                 std::uint64_t release) noexcept;

    std::array<std::atomic<double>, maxCandidates> candidateHz {};
    std::atomic<int> candidateCount { 0 };

    int currentMidi { -1 };
    double currentTargetHz {};
    int pendingMidi { -1 };
    int pendingCount {};
    std::uint64_t lastPitchSerial {};
    std::uint64_t lastOnsetSerial {};

    std::atomic<bool> publishedActive { false };
    std::atomic<int> publishedMidi { -1 };
    std::atomic<double> publishedTarget { 0.0 };
    std::atomic<double> publishedRaw { 0.0 };
    std::atomic<float> publishedCents { 0.0f };
    std::atomic<float> publishedConfidence { 0.0f };
    std::atomic<bool> publishedTransitioning { false };
    std::atomic<std::uint64_t> publishedOnsetSerial { 0 };
    std::atomic<std::uint64_t> publishedReleaseSerial { 0 };
};
} // namespace pmx::analysis
