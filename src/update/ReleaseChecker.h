#pragma once
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <vector>
#include "UpdateState.h"

namespace pmx::update
{
struct SemanticVersion
{
    int major { 0 }, minor { 0 }, patch { 0 };
    std::vector<std::string> prerelease;
    static std::optional<SemanticVersion> parse(std::string text);
    friend bool operator==(const SemanticVersion&, const SemanticVersion&) = default;
    friend bool operator<(const SemanticVersion& a, const SemanticVersion& b);
    friend bool operator>(const SemanticVersion& a, const SemanticVersion& b) { return b < a; }
};

class ReleaseChecker final
{
public:
    using Provider = std::function<std::vector<ReleaseInfo>(std::string& error)>;
    static UpdateDecision evaluate(const SemanticVersion& current, UpdateChannel, const std::vector<ReleaseInfo>&);
    static UpdateDecision offline(std::string message);
    static std::future<UpdateDecision> checkAsync(SemanticVersion current, UpdateChannel channel, Provider provider);
    static bool verifySha256(const std::filesystem::path&, const std::string& expectedDigest, std::string& error);
    static std::vector<ReleaseInfo> parseGitHubReleasesJson(const std::string& json, std::string& error);
    static bool isTrustedInstallerUrl(const std::string& url);
};
} // namespace pmx::update
