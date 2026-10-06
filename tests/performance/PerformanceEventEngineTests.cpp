#include <array>
#include <cmath>
#include "performance/PerformanceEventEngine.h"

namespace
{
using pmx::performance::PerformanceEvent;
using pmx::performance::PerformanceEventEngine;
using pmx::performance::PerformanceEventSource;
using pmx::performance::PerformanceEventType;

bool popExpect(PerformanceEventEngine& engine,
               PerformanceEventType type,
               PerformanceEventSource source,
               int midi)
{
    PerformanceEvent event;
    if(!engine.pop(event))return false;
    return event.type==type && event.source==source && event.midiNote==midi;
}

pmx::analysis::PolyphonicSnapshot emptyPoly(std::uint64_t serial)
{
    pmx::analysis::PolyphonicSnapshot p;
    p.serial=serial;
    return p;
}
}

int main()
{
    PerformanceEventEngine engine;
    engine.reset();

    pmx::analysis::ResolvedNoteSnapshot resolved;
    resolved.noteActive=true;
    resolved.midiNote=45;
    resolved.confidence=0.8f;
    resolved.centsFromTarget=0.0f;

    engine.process(resolved,emptyPoly(1));
    if(!popExpect(engine,PerformanceEventType::noteOn,PerformanceEventSource::resolved,45))return 1;
    if(!popExpect(engine,PerformanceEventType::pitchBend,PerformanceEventSource::resolved,45))return 2;
    if(engine.pendingCount()!=0)return 3;

    // Re-processing an unchanged note must not emit another NoteOn.
    engine.process(resolved,emptyPoly(1));
    if(engine.pendingCount()!=0)return 4;

    resolved.centsFromTarget=18.0f;
    engine.process(resolved,emptyPoly(1));
    PerformanceEvent event;
    if(!engine.pop(event) || event.type!=PerformanceEventType::pitchBend ||
       event.midiNote!=45 || std::abs(event.value-18.0f)>0.01f)return 5;
    if(engine.pendingCount()!=0)return 6;

    // Note changes are ordered NoteOff -> NoteOn -> initial bend.
    resolved.midiNote=47;
    resolved.centsFromTarget=-3.0f;
    engine.process(resolved,emptyPoly(1));
    if(!popExpect(engine,PerformanceEventType::noteOff,PerformanceEventSource::resolved,45))return 7;
    if(!popExpect(engine,PerformanceEventType::noteOn,PerformanceEventSource::resolved,47))return 8;
    if(!popExpect(engine,PerformanceEventType::pitchBend,PerformanceEventSource::resolved,47))return 9;

    resolved.noteActive=false;
    engine.process(resolved,emptyPoly(1));
    if(!popExpect(engine,PerformanceEventType::noteOff,PerformanceEventSource::resolved,47))return 10;

    engine.reset();

    pmx::analysis::PolyphonicSnapshot chord;
    chord.serial=2;
    chord.noteCount=3;
    chord.notes[0]={48,130.813,0.8f};
    chord.notes[1]={52,164.814,0.7f};
    chord.notes[2]={55,195.998,0.9f};

    engine.process({},chord);
    if(!popExpect(engine,PerformanceEventType::noteOn,PerformanceEventSource::polyphonic,48))return 11;
    if(!popExpect(engine,PerformanceEventType::noteOn,PerformanceEventSource::polyphonic,52))return 12;
    if(!popExpect(engine,PerformanceEventType::noteOn,PerformanceEventSource::polyphonic,55))return 13;
    if(!engine.pop(event) || event.type!=PerformanceEventType::chordChanged ||
       event.noteCount!=3 || event.notes[0]!=48 || event.notes[1]!=52 || event.notes[2]!=55)return 14;

    // Same detector frame cannot retrigger a chord.
    engine.process({},chord);
    if(engine.pendingCount()!=0)return 15;

    // Removing the middle note produces NoteOff before ChordChanged.
    chord.serial=3;
    chord.noteCount=2;
    chord.notes[0]={48,130.813,0.8f};
    chord.notes[1]={55,195.998,0.9f};
    engine.process({},chord);
    if(!popExpect(engine,PerformanceEventType::noteOff,PerformanceEventSource::polyphonic,52))return 16;
    if(!engine.pop(event) || event.type!=PerformanceEventType::chordChanged ||
       event.noteCount!=2 || event.notes[0]!=48 || event.notes[1]!=55)return 17;

    // Silence releases every active polyphonic note then publishes an empty chord.
    chord.serial=4;
    chord.noteCount=0;
    engine.process({},chord);
    if(!popExpect(engine,PerformanceEventType::noteOff,PerformanceEventSource::polyphonic,48))return 18;
    if(!popExpect(engine,PerformanceEventType::noteOff,PerformanceEventSource::polyphonic,55))return 19;
    if(!engine.pop(event) || event.type!=PerformanceEventType::chordChanged || event.noteCount!=0)return 20;

    // Overflow is bounded: no allocation or overwrite, just an observable drop count.
    engine.reset();
    pmx::analysis::ResolvedNoteSnapshot spam;
    spam.noteActive=true;
    spam.confidence=1.0f;
    for(int i=0;i<300;++i)
    {
        spam.midiNote=40+(i%20);
        spam.centsFromTarget=static_cast<float>((i%5)*4);
        engine.process(spam,emptyPoly(10));
    }
    if(engine.droppedCount()==0)return 21;
    if(engine.pendingCount()>=PerformanceEventEngine::queueCapacity)return 22;

    return 0;
}
