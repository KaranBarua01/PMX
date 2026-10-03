#pragma once
#include <cstdint>
#include <string>

namespace pmx::update
{
enum class UpdateChannel { stable, preview };
enum class UpdateStatus { upToDate, available, error };

struct ReleaseInfo
{
    std::string tagName;
    std::string name;
    bool prerelease { false };
    bool draft { false };
    std::string notes;
    std::string assetName;
    std::string assetUrl;
    std::uint64_t assetSize { 0 };
    std::string digest;
};

struct UpdateDecision
{
    UpdateStatus status { UpdateStatus::upToDate };
    std::string message;
    ReleaseInfo release;
};
} // namespace pmx::update
