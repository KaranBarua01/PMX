#include "TunerTap.h"
#include <algorithm>
#include <cmath>

namespace pmx::analysis
{
void TunerTap::prepare(std::size_t capacitySamples)
{
    ring.assign(std::max<std::size_t>(1, capacitySamples), 0.0f);
    writeIndex.store(0, std::memory_order_relaxed);
    readIndex.store(0, std::memory_order_relaxed);
}

std::size_t TunerTap::available() const noexcept
{
    const auto w = writeIndex.load(std::memory_order_acquire);
    const auto r = readIndex.load(std::memory_order_acquire);
    return static_cast<std::size_t>(w - r);
}

int TunerTap::push(const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0 || ring.empty()) return 0;
    auto w = writeIndex.load(std::memory_order_relaxed);
    const auto r = readIndex.load(std::memory_order_acquire);
    const auto used = static_cast<std::size_t>(w - r);
    const auto free = used < ring.size() ? ring.size() - used : 0;
    const auto count = static_cast<int>(std::min<std::size_t>(static_cast<std::size_t>(numSamples), free));
    for (int i = 0; i < count; ++i)
    {
        const float value = samples[i];
        ring[static_cast<std::size_t>(w % ring.size())] = std::isfinite(value) ? value : 0.0f;
        ++w;
    }
    writeIndex.store(w, std::memory_order_release);
    return count;
}

int TunerTap::pop(float* destination, int maxSamples) noexcept
{
    if (destination == nullptr || maxSamples <= 0 || ring.empty()) return 0;
    auto r = readIndex.load(std::memory_order_relaxed);
    const auto w = writeIndex.load(std::memory_order_acquire);
    const auto count = static_cast<int>(std::min<std::uint64_t>(static_cast<std::uint64_t>(maxSamples), w - r));
    for (int i = 0; i < count; ++i)
    {
        destination[i] = ring[static_cast<std::size_t>(r % ring.size())];
        ++r;
    }
    readIndex.store(r, std::memory_order_release);
    return count;
}
} // namespace pmx::analysis
