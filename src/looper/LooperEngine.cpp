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

void LooperEngine::commitPreviousOverdub()
{
    if (!hasOverdub) return;
    if (overdubApplied)
        for (std::size_t i = 0; i < loopLength; ++i)
        {
            left[i] += overdubLeft[i];
            right[i] += overdubRight[i];
        }
    std::fill(overdubLeft.begin(), overdubLeft.begin() + static_cast<std::ptrdiff_t>(loopLength), 0.0f);
    std::fill(overdubRight.begin(), overdubRight.begin() + static_cast<std::ptrdiff_t>(loopLength), 0.0f);
    hasOverdub = false;
    overdubApplied = false;
}

bool LooperEngine::overdub()
{
    if (loopLength == 0 || currentState != LooperState::playing) return false;
    commitPreviousOverdub(); // control-thread operation; never called by process().
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
    return true;
}

bool LooperEngine::redo() noexcept
{
    if (!canRedo() || (currentState != LooperState::playing && currentState != LooperState::stopped)) return false;
    overdubApplied = true;
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
                ++recordPosition;
            }
            if (recordPosition >= capacity)
                finaliseRecording(LooperState::playing);
            continue;
        }

        if ((currentState == LooperState::playing || currentState == LooperState::overdubbing) && loopLength > 0)
        {
            const auto index = position;
            const float layerL = hasOverdub && overdubApplied ? overdubLeft[index] : 0.0f;
            const float layerR = hasOverdub && overdubApplied ? overdubRight[index] : 0.0f;
            loopOutLeft[frame] = left[index] + layerL;
            loopOutRight[frame] = right[index] + layerR;

            if (currentState == LooperState::overdubbing)
            {
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
        const float layerL = hasOverdub && overdubApplied ? overdubLeft[i] : 0.0f;
        const float layerR = hasOverdub && overdubApplied ? overdubRight[i] : 0.0f;
        result.left[i] = left[i] + layerL;
        result.right[i] = right[i] + layerR;
    }
    return result;
}
} // namespace pmx::looper
