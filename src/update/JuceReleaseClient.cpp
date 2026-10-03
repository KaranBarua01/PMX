#include "JuceReleaseClient.h"
#include <juce_core/juce_core.h>

namespace pmx::update
{
namespace
{
juce::URL::InputStreamOptions networkOptions(int* status)
{
    return juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000)
        .withNumRedirectsToFollow(5)
        .withStatusCode(status)
        .withExtraHeaders("Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\nUser-Agent: PMX-Updater\r\n");
}
}

std::vector<ReleaseInfo> JuceReleaseClient::fetchReleases(std::string& error)
{
    int status = 0;
    juce::URL url("https://api.github.com/repos/KaranBarua01/PMX/releases?per_page=20");
    auto stream = url.createInputStream(networkOptions(&status));
    if (!stream)
    {
        error = "Could not reach GitHub. PMX can still be used offline.";
        return {};
    }
    if (status < 200 || status >= 300)
    {
        error = "GitHub returned HTTP " + std::to_string(status) + ".";
        return {};
    }
    return ReleaseChecker::parseGitHubReleasesJson(stream->readEntireStreamAsString().toStdString(), error);
}

std::future<DownloadResult> JuceReleaseClient::downloadInstallerAsync(ReleaseInfo release, std::filesystem::path destination)
{
    return std::async(std::launch::async, [release = std::move(release), destination = std::move(destination)]() mutable -> DownloadResult {
        if (!ReleaseChecker::isTrustedInstallerUrl(release.assetUrl))
            return {false, "The release installer URL is not trusted.", {}};
        if (release.digest.empty())
            return {false, "The release has no SHA-256 digest.", {}};

        std::error_code ec;
        std::filesystem::create_directories(destination.parent_path(), ec);
        if (ec) return {false, "Update staging folder could not be created.", {}};
        const auto partial = std::filesystem::path(destination.string() + ".partial");
        std::filesystem::remove(partial, ec);

        int status = 0;
        auto stream = juce::URL(juce::String(release.assetUrl)).createInputStream(networkOptions(&status));
        if (!stream || status < 200 || status >= 300)
            return {false, "Installer download failed.", {}};

        juce::File file(juce::String(partial.string()));
        auto output = std::make_unique<juce::FileOutputStream>(file);
        if (output->failedToOpen()) return {false, "Downloaded installer could not be created.", {}};
        const auto written = output->writeFromInputStream(*stream, -1);
        output->flush();
        output.reset();
        if (written < 0 || (release.assetSize != 0 && static_cast<std::uint64_t>(written) != release.assetSize))
        {
            std::filesystem::remove(partial, ec);
            return {false, "Installer download was incomplete.", {}};
        }

        std::string digestError;
        if (!ReleaseChecker::verifySha256(partial, release.digest, digestError))
        {
            std::filesystem::remove(partial, ec);
            return {false, digestError, {}};
        }
        std::filesystem::remove(destination, ec);
        ec.clear();
        std::filesystem::rename(partial, destination, ec);
        if (ec) return {false, "Verified installer could not be staged.", {}};
        return {true, {}, destination};
    });
}

bool JuceReleaseClient::launchInstaller(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) return false;
    return juce::File(juce::String(path.string())).startAsProcess();
}
} // namespace pmx::update
