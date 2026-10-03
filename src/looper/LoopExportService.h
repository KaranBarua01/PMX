#pragma once
#include <filesystem>
#include <future>
#include <string>
#include "LooperEngine.h"

namespace pmx::looper
{
struct LoopExportResult
{
    bool ok { false };
    std::string error;
    std::filesystem::path path;
};

class LoopExportService final
{
public:
    static std::future<LoopExportResult> writeWav24Async(LoopSnapshot snapshot, std::filesystem::path path);

private:
    static LoopExportResult writeWav24(const LoopSnapshot& snapshot, const std::filesystem::path& path);
};
} // namespace pmx::looper
