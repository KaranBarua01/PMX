#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pmx::analysis
{
// Single-producer/single-consumer FIFO. The audio callback is the producer;
// the UI timer is the consumer. No allocation or locking occurs in push/pop.
class TunerTap final
{
public:
    void prepare(std::size_t capacitySamples);
    int push(const float* samples, int numSamples) noexcept;
    int pop(float* destination, int maxSamples) noexcept;
    [[nodiscard]] std::size_t available() const noexcept;

private:
    std::vector<float> ring;
    std::atomic<std::uint64_t> writeIndex { 0 };
    std::atomic<std::uint64_t> readIndex { 0 };
};
} // namespace pmx::analysis
