#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include "analysis/FretboardNoteSolver.h"
#include "analysis/PolyphonicNoteDetector.h"

namespace pmx::performance
{
enum class PerformanceEventType : std::uint8_t
{
    noteOn,
    noteOff,
    pitchBend,
    chordChanged
};

enum class PerformanceEventSource : std::uint8_t
{
    resolved,
    polyphonic
};

struct PerformanceEvent
{
    PerformanceEventType type { PerformanceEventType::noteOff };
    PerformanceEventSource source { PerformanceEventSource::resolved };
    int midiNote { -1 };
    float value {};
    int noteCount {};
    std::array<int,analysis::PolyphonicSnapshot::maxNotes> notes {};
    std::uint64_t sequence {};
};

class PerformanceEventSink
{
public:
    virtual ~PerformanceEventSink()=default;
    virtual void handlePerformanceEvent(const PerformanceEvent& event) noexcept=0;
};

class PerformanceEventEngine final
{
public:
    static constexpr unsigned queueCapacity=128;

    void reset() noexcept;
    void setSink(PerformanceEventSink* newSink) noexcept { sink=newSink; }
    void process(const analysis::ResolvedNoteSnapshot& resolved,
                 const analysis::PolyphonicSnapshot& polyphonic) noexcept;

    [[nodiscard]] bool pop(PerformanceEvent& event) noexcept;
    [[nodiscard]] unsigned pendingCount() const noexcept;
    [[nodiscard]] std::uint64_t droppedCount() const noexcept
    {
        return dropped.load(std::memory_order_relaxed);
    }

private:
    void processResolved(const analysis::ResolvedNoteSnapshot& resolved) noexcept;
    void processPolyphonic(const analysis::PolyphonicSnapshot& polyphonic) noexcept;
    bool push(PerformanceEvent event) noexcept;
    static float safeUnit(float value) noexcept;
    static float safeCents(float value) noexcept;

    PerformanceEventSink* sink {};
    std::array<PerformanceEvent,queueCapacity> queue {};
    std::atomic<unsigned> writeIndex { 0 };
    std::atomic<unsigned> readIndex { 0 };
    std::atomic<std::uint64_t> dropped { 0 };
    std::uint64_t nextSequence {};

    bool resolvedActive {};
    int resolvedMidi { -1 };
    float lastResolvedBend {};
    bool haveResolvedBend {};

    std::array<int,analysis::PolyphonicSnapshot::maxNotes> polyphonicNotes {};
    int polyphonicCount {};
    std::uint64_t lastPolyphonicSerial {};
};
} // namespace pmx::performance
