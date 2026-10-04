#pragma once
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace pmx::recording
{
struct RecorderResult
{
    bool ok { false };
    std::string error;
    std::filesystem::path path;
};

class QuickRecorder final
{
public:
    QuickRecorder() = default;
    ~QuickRecorder();
    QuickRecorder(const QuickRecorder&) = delete;
    QuickRecorder& operator=(const QuickRecorder&) = delete;

    void prepare(double sampleRate, int channels = 2, double ringSeconds = 2.0);
    RecorderResult start(const std::filesystem::path& outputPath);
    bool push(const float* left, const float* right, int frames) noexcept;
    RecorderResult stop();
    RecorderResult interruptForRecovery();

    [[nodiscard]] bool isRecording() const noexcept { return recording.load(std::memory_order_acquire); }
    [[nodiscard]] bool needsFinalisation() const noexcept {return !isRecording()&&worker.joinable();}
    [[nodiscard]] std::uint64_t framesAccepted() const noexcept { return accepted.load(std::memory_order_relaxed); }
    [[nodiscard]] std::uint64_t droppedFrames() const noexcept { return dropped.load(std::memory_order_relaxed); }

private:
    void workerLoop();
    void writeHeader(std::uint32_t dataBytes);
    void patchHeader();
    static void writeSample24(std::ostream&, float);

    double rate { 0.0 };
    int channelCount { 2 };
    std::size_t capacityFrames { 0 };
    std::vector<float> ring;
    std::atomic<std::uint64_t> writeFrame { 0 };
    std::atomic<std::uint64_t> readFrame { 0 };
    std::atomic<std::uint64_t> accepted { 0 };
    std::atomic<std::uint64_t> dropped { 0 };
    std::atomic<bool> recording { false };
    std::atomic<unsigned> activeProducers {0};
    std::thread worker;
    std::ofstream output;
    std::filesystem::path path;
    std::uint64_t framesWritten { 0 };
    std::atomic<bool> writerFailed{false};
};
} // namespace pmx::recording
