#pragma once
#include <cstddef>
#include <vector>

namespace pmx::looper
{
enum class LooperState { empty, recording, playing, overdubbing, stopped };

struct LoopSnapshot
{
    double sampleRate { 0.0 };
    std::vector<float> left;
    std::vector<float> right;
};

class LooperEngine final
{
public:
    void prepare(double sampleRate, double maxSeconds = 120.0);
    bool record() noexcept;
    bool play() noexcept;
    bool overdub();
    bool stop() noexcept;
    bool undo() noexcept;
    bool redo() noexcept;
    void clear() noexcept;
    void rewind() noexcept { position = 0; }

    void process(const float* inputLeft, const float* inputRight,
                 float* loopOutLeft, float* loopOutRight, int numFrames) noexcept;

    [[nodiscard]] LooperState state() const noexcept { return currentState; }
    [[nodiscard]] std::size_t capacityFrames() const noexcept { return capacity; }
    [[nodiscard]] std::size_t loopFrames() const noexcept { return loopLength; }
    [[nodiscard]] std::size_t recordedFrames() const noexcept { return currentState == LooperState::recording ? recordPosition : loopLength; }
    [[nodiscard]] bool canUndo() const noexcept { return hasOverdub && overdubApplied; }
    [[nodiscard]] bool canRedo() const noexcept { return hasOverdub && !overdubApplied; }
    [[nodiscard]] LoopSnapshot snapshot() const;

private:
    void finaliseRecording(LooperState next) noexcept;
    void finaliseOverdub(LooperState next) noexcept;
    void commitPreviousOverdub();

    double rate { 0.0 };
    std::size_t capacity { 0 };
    std::size_t loopLength { 0 };
    std::size_t recordPosition { 0 };
    std::size_t position { 0 };
    LooperState currentState { LooperState::empty };
    std::vector<float> left;
    std::vector<float> right;
    std::vector<float> overdubLeft;
    std::vector<float> overdubRight;
    bool hasOverdub { false };
    bool overdubApplied { false };
};
} // namespace pmx::looper
