#include "LooperEngine.h"
#include <algorithm>
#include <cmath>

namespace pmx::looper
{
void LooperEngine::prepare(double sampleRate, double maxSeconds)
{
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto seconds = std::clamp(maxSeconds, 0.1, 120.0);
    capacity = static_cast<std::size_t>(std::ceil(rate * seconds));
    left.assign(capacity, 0.0f);
    right.assign(capacity, 0.0f);
    overdubLeft.assign(capacity, 0.0f);
    overdubRight.assign(capacity, 0.0f);
    layerTags.assign(capacity, 0);
    clear();
}

bool LooperEngine::record() noexcept
{
    if (capacity == 0 || currentState != LooperState::empty) return false;
    currentState = LooperState::recording;
    recordPosition = 0;
    position = 0;
    return true;
}

void LooperEngine::finaliseRecording(LooperState next) noexcept
{
    if (recordPosition == 0)
    {
        currentState = LooperState::empty;
        loopLength = 0;
        position = 0;
        return;
    }
    loopLength = std::min(recordPosition, capacity);
    recordPosition = loopLength;
    position = 0;
    currentState = next;
}

void LooperEngine::finaliseOverdub(LooperState next) noexcept
{
    hasOverdub = true;
    overdubApplied = true;
    currentState = next;
}

bool LooperEngine::play() noexcept
{
    if (currentState == LooperState::recording)
    {
        finaliseRecording(LooperState::playing);
        return currentState == LooperState::playing;
    }
    if (currentState == LooperState::overdubbing)
    {
        finaliseOverdub(LooperState::playing);
        return true;
    }
    if (currentState == LooperState::stopped && loopLength > 0)
    {
        position = 0;
        currentState = LooperState::playing;
        return true;
    }
    return currentState == LooperState::playing;
}

float LooperEngine::layerSample(std::size_t index, bool rightChannel) const noexcept
{
    const auto tag = layerTags[index];
    const auto& epoch = epochs[tag % epochSlots];
    return tag != 0 && epoch.id == tag && epoch.applied
        ? (rightChannel ? overdubRight[index] : overdubLeft[index]) : 0.0f;
}

void LooperEngine::settleLayer(std::size_t index) noexcept
{
    const auto tag = layerTags[index];
    if (tag == generation) return;
    if (tag != 0)
    {
        auto& epoch = epochs[tag % epochSlots];
        if (epoch.id == tag)
        {
            if (epoch.applied) { left[index] += overdubLeft[index]; right[index] += overdubRight[index]; }
            if (epoch.references > 0) --epoch.references;
        }
    }
    layerTags[index] = 0;
}

bool LooperEngine::overdub()
{
    if (loopLength == 0 || currentState != LooperState::playing) return false;
    // Commit older layers lazily as samples are visited, never copy a whole loop.
    const auto next = generation + 1;
    auto& epoch = epochs[next % epochSlots];
    if (epoch.references != 0) return false;
    generation = next;
    epoch = {generation, 0, true};
    hasOverdub = false;
    overdubApplied = false;
    currentState = LooperState::overdubbing;
    return true;
}

bool LooperEngine::stop() noexcept
{
    if (currentState == LooperState::recording)
    {
        finaliseRecording(LooperState::stopped);
        return currentState == LooperState::stopped;
    }
    if (currentState == LooperState::overdubbing)
    {
        finaliseOverdub(LooperState::stopped);
        return true;
    }
    if (currentState == LooperState::playing)
    {
        currentState = LooperState::stopped;
        return true;
    }
    return currentState == LooperState::stopped;
}

bool LooperEngine::undo() noexcept
{
    if (!canUndo() || (currentState != LooperState::playing && currentState != LooperState::stopped)) return false;
    overdubApplied = false;
    epochs[generation % epochSlots].applied = false;
    return true;
}

bool LooperEngine::redo() noexcept
{
    if (!canRedo() || (currentState != LooperState::playing && currentState != LooperState::stopped)) return false;
    overdubApplied = true;
    epochs[generation % epochSlots].applied = true;
    return true;
}

void LooperEngine::clear() noexcept
{
    currentState = LooperState::empty;
    loopLength = 0;
    recordPosition = 0;
    position = 0;
    hasOverdub = false;
    overdubApplied = false;
    ++generation;
    epochs.fill({}); // fixed metadata; independent of 120-second audio storage
}

void LooperEngine::process(const float* inputLeft, const float* inputRight,
                           float* loopOutLeft, float* loopOutRight, int numFrames) noexcept
{
    if (numFrames <= 0 || !loopOutLeft || !loopOutRight) return;
    for (int frame = 0; frame < numFrames; ++frame)
    {
        loopOutLeft[frame] = 0.0f;
        loopOutRight[frame] = 0.0f;

        if (currentState == LooperState::recording)
        {
            if (recordPosition < capacity)
            {
                left[recordPosition] = inputLeft ? inputLeft[frame] : 0.0f;
                right[recordPosition] = inputRight ? inputRight[frame] : (inputLeft ? inputLeft[frame] : 0.0f);
                layerTags[recordPosition] = 0;
                ++recordPosition;
            }
            if (recordPosition >= capacity)
                finaliseRecording(LooperState::playing);
            continue;
        }

        if ((currentState == LooperState::playing || currentState == LooperState::overdubbing) && loopLength > 0)
        {
            const auto index = position;
            settleLayer(index);
            const float layerL = layerSample(index, false);
            const float layerR = layerSample(index, true);
            loopOutLeft[frame] = left[index] + layerL;
            loopOutRight[frame] = right[index] + layerR;

            if (currentState == LooperState::overdubbing)
            {
                if (layerTags[index] != generation)
                {
                    layerTags[index] = generation;
                    overdubLeft[index] = overdubRight[index] = 0.0f;
                    ++epochs[generation % epochSlots].references;
                }
                overdubLeft[index] += inputLeft ? inputLeft[frame] : 0.0f;
                overdubRight[index] += inputRight ? inputRight[frame] : (inputLeft ? inputLeft[frame] : 0.0f);
            }
            position = (position + 1) % loopLength;
        }
    }
}

LoopSnapshot LooperEngine::snapshot() const
{
    LoopSnapshot result;
    result.sampleRate = rate;
    if (loopLength == 0) return result;
    result.left.resize(loopLength);
    result.right.resize(loopLength);
    for (std::size_t i = 0; i < loopLength; ++i)
    {
        const float layerL = layerSample(i, false);
        const float layerR = layerSample(i, true);
        result.left[i] = left[i] + layerL;
        result.right[i] = right[i] + layerR;
    }
    return result;
}
} // namespace pmx::looper
