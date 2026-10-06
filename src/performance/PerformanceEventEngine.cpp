#include "performance/PerformanceEventEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::performance
{
namespace
{
bool contains(const std::array<int,analysis::PolyphonicSnapshot::maxNotes>& notes,
              int count,
              int midi) noexcept
{
    for(int i=0;i<count;++i)
        if(notes[static_cast<std::size_t>(i)]==midi)return true;
    return false;
}
}

float PerformanceEventEngine::safeUnit(float value) noexcept
{
    return std::isfinite(value)?std::clamp(value,0.0f,1.0f):0.0f;
}

float PerformanceEventEngine::safeCents(float value) noexcept
{
    return std::isfinite(value)?std::clamp(value,-200.0f,200.0f):0.0f;
}

void PerformanceEventEngine::reset() noexcept
{
    const auto write=writeIndex.load(std::memory_order_relaxed);
    readIndex.store(write,std::memory_order_release);
    dropped.store(0,std::memory_order_relaxed);
    nextSequence=0;

    resolvedActive=false;
    resolvedMidi=-1;
    lastResolvedBend=0.0f;
    haveResolvedBend=false;

    polyphonicNotes.fill(-1);
    polyphonicCount=0;
    lastPolyphonicSerial=0;
}

bool PerformanceEventEngine::push(PerformanceEvent event) noexcept
{
    event.sequence=++nextSequence;
    if(sink!=nullptr)sink->handlePerformanceEvent(event);

    const auto write=writeIndex.load(std::memory_order_relaxed);
    const auto next=(write+1u)%queueCapacity;
    if(next==readIndex.load(std::memory_order_acquire))
    {
        dropped.fetch_add(1,std::memory_order_relaxed);
        return false;
    }

    queue[write]=event;
    writeIndex.store(next,std::memory_order_release);
    return true;
}

bool PerformanceEventEngine::pop(PerformanceEvent& event) noexcept
{
    const auto read=readIndex.load(std::memory_order_relaxed);
    if(read==writeIndex.load(std::memory_order_acquire))return false;

    event=queue[read];
    readIndex.store((read+1u)%queueCapacity,std::memory_order_release);
    return true;
}

unsigned PerformanceEventEngine::pendingCount() const noexcept
{
    const auto read=readIndex.load(std::memory_order_acquire);
    const auto write=writeIndex.load(std::memory_order_acquire);
    return write>=read?write-read:queueCapacity-read+write;
}

void PerformanceEventEngine::process(
    const analysis::ResolvedNoteSnapshot& resolved,
    const analysis::PolyphonicSnapshot& polyphonic) noexcept
{
    processResolved(resolved);
    processPolyphonic(polyphonic);
}

void PerformanceEventEngine::processResolved(
    const analysis::ResolvedNoteSnapshot& resolved) noexcept
{
    const bool active=resolved.noteActive && resolved.midiNote>=0 && resolved.midiNote<128;

    if(!active)
    {
        if(resolvedActive)
        {
            PerformanceEvent event;
            event.type=PerformanceEventType::noteOff;
            event.source=PerformanceEventSource::resolved;
            event.midiNote=resolvedMidi;
            push(event);
        }

        resolvedActive=false;
        resolvedMidi=-1;
        haveResolvedBend=false;
        return;
    }

    if(!resolvedActive || resolved.midiNote!=resolvedMidi)
    {
        if(resolvedActive)
        {
            PerformanceEvent off;
            off.type=PerformanceEventType::noteOff;
            off.source=PerformanceEventSource::resolved;
            off.midiNote=resolvedMidi;
            push(off);
        }

        PerformanceEvent on;
        on.type=PerformanceEventType::noteOn;
        on.source=PerformanceEventSource::resolved;
        on.midiNote=resolved.midiNote;
        on.value=safeUnit(resolved.confidence);
        push(on);

        resolvedActive=true;
        resolvedMidi=resolved.midiNote;
        haveResolvedBend=false;
    }

    const float cents=safeCents(resolved.centsFromTarget);
    if(!haveResolvedBend || std::abs(cents-lastResolvedBend)>=1.5f)
    {
        PerformanceEvent bend;
        bend.type=PerformanceEventType::pitchBend;
        bend.source=PerformanceEventSource::resolved;
        bend.midiNote=resolvedMidi;
        bend.value=cents;
        push(bend);
        lastResolvedBend=cents;
        haveResolvedBend=true;
    }
}

void PerformanceEventEngine::processPolyphonic(
    const analysis::PolyphonicSnapshot& polyphonic) noexcept
{
    if(polyphonic.serial==lastPolyphonicSerial)return;
    lastPolyphonicSerial=polyphonic.serial;

    std::array<int,analysis::PolyphonicSnapshot::maxNotes> nextNotes {};
    nextNotes.fill(-1);
    int nextCount=0;

    const int sourceCount=std::clamp(polyphonic.noteCount,0,analysis::PolyphonicSnapshot::maxNotes);
    for(int i=0;i<sourceCount && nextCount<analysis::PolyphonicSnapshot::maxNotes;++i)
    {
        const int midi=polyphonic.notes[static_cast<std::size_t>(i)].midiNote;
        if(midi<0||midi>=128||contains(nextNotes,nextCount,midi))continue;
        nextNotes[static_cast<std::size_t>(nextCount++)]=midi;
    }
    std::sort(nextNotes.begin(),nextNotes.begin()+nextCount);

    bool changed=nextCount!=polyphonicCount;
    if(!changed)
        for(int i=0;i<nextCount;++i)
            if(nextNotes[static_cast<std::size_t>(i)]!=polyphonicNotes[static_cast<std::size_t>(i)])
            {
                changed=true;
                break;
            }

    if(!changed)return;

    // Release notes before starting new notes so consumers can update voices
    // deterministically during fast chord changes.
    for(int i=0;i<polyphonicCount;++i)
    {
        const int midi=polyphonicNotes[static_cast<std::size_t>(i)];
        if(contains(nextNotes,nextCount,midi))continue;

        PerformanceEvent off;
        off.type=PerformanceEventType::noteOff;
        off.source=PerformanceEventSource::polyphonic;
        off.midiNote=midi;
        push(off);
    }

    for(int i=0;i<nextCount;++i)
    {
        const int midi=nextNotes[static_cast<std::size_t>(i)];
        if(contains(polyphonicNotes,polyphonicCount,midi))continue;

        float confidence=0.0f;
        for(int j=0;j<sourceCount;++j)
            if(polyphonic.notes[static_cast<std::size_t>(j)].midiNote==midi)
            {
                confidence=polyphonic.notes[static_cast<std::size_t>(j)].confidence;
                break;
            }

        PerformanceEvent on;
        on.type=PerformanceEventType::noteOn;
        on.source=PerformanceEventSource::polyphonic;
        on.midiNote=midi;
        on.value=safeUnit(confidence);
        push(on);
    }

    PerformanceEvent chord;
    chord.type=PerformanceEventType::chordChanged;
    chord.source=PerformanceEventSource::polyphonic;
    chord.noteCount=nextCount;
    chord.notes=nextNotes;
    push(chord);

    polyphonicNotes=nextNotes;
    polyphonicCount=nextCount;
}
} // namespace pmx::performance
