#include "LoopExportService.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>

namespace pmx::looper
{
namespace
{
void writeU16(std::ostream& out, std::uint16_t v)
{
    out.put(static_cast<char>(v & 0xff));
    out.put(static_cast<char>((v >> 8) & 0xff));
}
void writeU32(std::ostream& out, std::uint32_t v)
{
    for (int i = 0; i < 4; ++i) out.put(static_cast<char>((v >> (8 * i)) & 0xff));
}
void writeS24(std::ostream& out, float sample)
{
    sample = std::clamp(sample, -1.0f, 1.0f);
    const auto scaled = static_cast<std::int32_t>(std::lrint(sample * 8388607.0f));
    const auto bits = static_cast<std::uint32_t>(scaled);
    out.put(static_cast<char>(bits & 0xff));
    out.put(static_cast<char>((bits >> 8) & 0xff));
    out.put(static_cast<char>((bits >> 16) & 0xff));
}
}

std::future<LoopExportResult> LoopExportService::writeWav24Async(LoopSnapshot snapshot, std::filesystem::path path)
{
    return std::async(std::launch::async, [snapshot = std::move(snapshot), path = std::move(path)]() mutable {
        return writeWav24(snapshot, path);
    });
}

LoopExportResult LoopExportService::writeWav24(const LoopSnapshot& snapshot, const std::filesystem::path& path)
{
    if (snapshot.sampleRate <= 0.0 || snapshot.left.empty() || snapshot.left.size() != snapshot.right.size())
        return { false, "Loop is empty or invalid.", {} };

    std::error_code ec;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return { false, "Export folder could not be created.", {} };

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return { false, "WAV file could not be created.", {} };

    constexpr std::uint16_t channels = 2;
    constexpr std::uint16_t bitsPerSample = 24;
    const auto sampleRate = static_cast<std::uint32_t>(std::llround(snapshot.sampleRate));
    const auto frameCount = static_cast<std::uint32_t>(snapshot.left.size());
    const auto blockAlign = static_cast<std::uint16_t>(channels * bitsPerSample / 8);
    const auto byteRate = sampleRate * blockAlign;
    const auto dataBytes = frameCount * blockAlign;
    const auto riffBytes = 36u + dataBytes;

    out.write("RIFF", 4); writeU32(out, riffBytes); out.write("WAVE", 4);
    out.write("fmt ", 4); writeU32(out, 16); writeU16(out, 1); writeU16(out, channels);
    writeU32(out, sampleRate); writeU32(out, byteRate); writeU16(out, blockAlign); writeU16(out, bitsPerSample);
    out.write("data", 4); writeU32(out, dataBytes);
    for (std::size_t i = 0; i < snapshot.left.size(); ++i)
    {
        writeS24(out, snapshot.left[i]);
        writeS24(out, snapshot.right[i]);
    }
    out.flush();
    if (!out) return { false, "WAV export failed while writing.", {} };
    return { true, {}, path };
}
} // namespace pmx::looper
