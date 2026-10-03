#pragma once
#include <filesystem>
#include <future>
#include <string>
#include <vector>
#include "ReleaseChecker.h"

namespace pmx::update
{
struct DownloadResult
{
    bool ok { false };
    std::string error;
    std::filesystem::path path;
};

class JuceReleaseClient final
{
public:
    static std::vector<ReleaseInfo> fetchReleases(std::string& error);
    static std::future<DownloadResult> downloadInstallerAsync(ReleaseInfo release, std::filesystem::path destination);
    static bool launchInstaller(const std::filesystem::path& path);
};
} // namespace pmx::update
