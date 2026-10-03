#include "QuickRecorder.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace pmx::recording
{
namespace
{
void u16(std::ostream& out, std::uint16_t v)
{
    out.put(static_cast<char>(v & 0xff)); out.put(static_cast<char>((v >> 8) & 0xff));
}
void u32(std::ostream& out, std::uint32_t v)
{
    for(int i=0;i<4;++i) out.put(static_cast<char>((v >> (8*i)) & 0xff));
}
}

QuickRecorder::~QuickRecorder()
{
    if (isRecording()) interruptForRecovery();
    else if (worker.joinable()) worker.join();
}

void QuickRecorder::prepare(double sampleRate, int channels, double ringSeconds)
{
    if (isRecording()) stop();
    rate = sampleRate > 0.0 ? sampleRate : 44100.0;
    channelCount = channels <= 1 ? 1 : 2;
    capacityFrames = static_cast<std::size_t>(std::max(256.0, std::ceil(rate * std::max(0.25, ringSeconds))));
    ring.assign(capacityFrames * 2u, 0.0f);
    writeFrame.store(0); readFrame.store(0); accepted.store(0); dropped.store(0);
}

RecorderResult QuickRecorder::start(const std::filesystem::path& outputPath)
{
    if (isRecording()) return {false, "A recording is already running.", {}};
    if (worker.joinable()) worker.join();
    if (capacityFrames == 0) return {false, "Recorder has not been prepared.", {}};
    if (outputPath.empty() || !outputPath.has_parent_path() || !std::filesystem::exists(outputPath.parent_path()))
        return {false, "Choose an existing folder for the recording.", {}};

    path = outputPath;
    output.open(path, std::ios::binary | std::ios::trunc);
    if (!output) return {false, "Recording file could not be created.", {}};
    framesWritten = 0;
    writeFrame.store(0, std::memory_order_relaxed);
    readFrame.store(0, std::memory_order_relaxed);
    accepted.store(0, std::memory_order_relaxed);
    dropped.store(0, std::memory_order_relaxed);
    writeHeader(0);
    output.flush();
    if (!output) { output.close(); return {false, "Recording header could not be written.", {}}; }
    recording.store(true, std::memory_order_release);
    worker = std::thread([this]{ workerLoop(); });
    return {true, {}, path};
}

bool QuickRecorder::push(const float* left, const float* right, int frames) noexcept
{
    if (!isRecording() || !left || frames <= 0) return false;
    const auto write = writeFrame.load(std::memory_order_relaxed);
    const auto read = readFrame.load(std::memory_order_acquire);
    const auto count = static_cast<std::uint64_t>(frames);
    if (count > capacityFrames || write - read + count > capacityFrames)
    {
        dropped.fetch_add(count, std::memory_order_relaxed);
        return false;
    }
    for (int i=0;i<frames;++i)
    {
        const auto index = static_cast<std::size_t>((write + static_cast<std::uint64_t>(i)) % capacityFrames) * 2u;
        ring[index] = left[i];
        ring[index+1] = right ? right[i] : left[i];
    }
    writeFrame.store(write + count, std::memory_order_release);
    accepted.fetch_add(count, std::memory_order_relaxed);
    return true;
}

void QuickRecorder::writeSample24(std::ostream& out, float sample)
{
    sample = std::clamp(sample, -1.0f, 1.0f);
    const auto value = static_cast<std::int32_t>(std::lrint(sample * 8388607.0f));
    const auto bits = static_cast<std::uint32_t>(value);
    out.put(static_cast<char>(bits & 0xff));
    out.put(static_cast<char>((bits >> 8) & 0xff));
    out.put(static_cast<char>((bits >> 16) & 0xff));
}

void QuickRecorder::writeHeader(std::uint32_t dataBytes)
{
    const auto sr = static_cast<std::uint32_t>(std::llround(rate));
    const auto channels = static_cast<std::uint16_t>(channelCount);
    constexpr std::uint16_t bits = 24;
    const auto align = static_cast<std::uint16_t>(channels * bits / 8);
    output.write("RIFF",4); u32(output,36u+dataBytes); output.write("WAVE",4);
    output.write("fmt ",4); u32(output,16); u16(output,1); u16(output,channels);
    u32(output,sr); u32(output,sr*align); u16(output,align); u16(output,bits);
    output.write("data",4); u32(output,dataBytes);
}

void QuickRecorder::patchHeader()
{
    const auto bytesPerFrame = static_cast<std::uint32_t>(channelCount * 3);
    const auto dataBytes = static_cast<std::uint32_t>(std::min<std::uint64_t>(framesWritten * bytesPerFrame, 0xffffffffu - 44u));
    output.flush(); output.clear();
    output.seekp(4, std::ios::beg); u32(output, 36u + dataBytes);
    output.seekp(40, std::ios::beg); u32(output, dataBytes);
    output.seekp(0, std::ios::end);
    output.flush();
}

void QuickRecorder::workerLoop()
{
    std::uint64_t lastPatched = 0;
    while (recording.load(std::memory_order_acquire) || readFrame.load(std::memory_order_relaxed) < writeFrame.load(std::memory_order_acquire))
    {
        auto read = readFrame.load(std::memory_order_relaxed);
        const auto write = writeFrame.load(std::memory_order_acquire);
        if (read == write)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        const auto batchEnd = std::min(write, read + 512u);
        while (read < batchEnd)
        {
            const auto index = static_cast<std::size_t>(read % capacityFrames) * 2u;
            writeSample24(output, ring[index]);
            if (channelCount > 1) writeSample24(output, ring[index+1]);
            ++read; ++framesWritten;
        }
        readFrame.store(read, std::memory_order_release);
        if (framesWritten - lastPatched >= 1024)
        {
            patchHeader();
            lastPatched = framesWritten;
        }
    }
    patchHeader();
}

RecorderResult QuickRecorder::stop()
{
    if (!isRecording()) return {false, "No recording is running.", path};
    recording.store(false, std::memory_order_release);
    if (worker.joinable()) worker.join();
    patchHeader();
    output.close();
    if (!std::filesystem::exists(path) || std::filesystem::file_size(path) <= 44)
        return {false, "Recording ended before audio was written.", path};
    return {true, {}, path};
}

RecorderResult QuickRecorder::interruptForRecovery()
{
    if (!isRecording()) return {false, "No recording is running.", path};
    recording.store(false, std::memory_order_release);
    if (worker.joinable()) worker.join();
    patchHeader();
    output.close();
    return {std::filesystem::exists(path) && std::filesystem::file_size(path) > 44,
            std::filesystem::exists(path) ? std::string{} : std::string{"Recovery file was not created."}, path};
}
} // namespace pmx::recording
