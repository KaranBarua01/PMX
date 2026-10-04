#include "JuceReleaseClient.h"
#include <fstream>
#include <juce_core/juce_core.h>

#if defined(_WIN32)
#include <windows.h>
#include <winhttp.h>
#endif

namespace pmx::update
{
namespace
{
#if defined(_WIN32)
class WinHttpHandle
{
public:
    explicit WinHttpHandle(HINTERNET value = nullptr) : handle(value) {}
    ~WinHttpHandle() { if (handle) WinHttpCloseHandle(handle); }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    operator HINTERNET() const noexcept { return handle; }
    [[nodiscard]] bool valid() const noexcept { return handle != nullptr; }
private:
    HINTERNET handle {};
};

struct HttpResponse
{
    bool ok { false };
    DWORD status { 0 };
    DWORD systemError { 0 };
    std::string body;
};

bool crackUrl(const std::string& url, std::wstring& host, std::wstring& path, INTERNET_PORT& port, bool& secure)
{
    const juce::String wideUrl(url);
    const std::wstring value(wideUrl.toWideCharPointer());

    URL_COMPONENTS parts {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(value.c_str(), 0, 0, &parts))
        return false;

    host.assign(parts.lpszHostName, parts.dwHostNameLength);
    path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.lpszExtraInfo && parts.dwExtraInfoLength > 0)
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    if (path.empty()) path = L"/";

    port = parts.nPort;
    secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
    return !host.empty();
}

HttpResponse httpGet(const std::string& url)
{
    HttpResponse result;
    std::wstring host, path;
    INTERNET_PORT port = 0;
    bool secure = false;
    if (!crackUrl(url, host, path, port, secure))
    {
        result.systemError = ERROR_INVALID_PARAMETER;
        return result;
    }

    WinHttpHandle session(WinHttpOpen(L"PMX-Updater/0.2",
                                      WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.valid())
    {
        result.systemError = GetLastError();
        return result;
    }

    WinHttpSetTimeouts(session, 10000, 10000, 10000, 15000);

    WinHttpHandle connection(WinHttpConnect(session, host.c_str(), port, 0));
    if (!connection.valid())
    {
        result.systemError = GetLastError();
        return result;
    }

    const DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    WinHttpHandle request(WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
                                             WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request.valid())
    {
        result.systemError = GetLastError();
        return result;
    }

    constexpr wchar_t headers[] =
        L"Accept: application/vnd.github+json\r\n"
        L"X-GitHub-Api-Version: 2022-11-28\r\n";

    if (!WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L),
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(request, nullptr))
    {
        result.systemError = GetLastError();
        return result;
    }

    DWORD size = sizeof(result.status);
    if (!WinHttpQueryHeaders(request,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &result.status, &size, WINHTTP_NO_HEADER_INDEX))
    {
        result.systemError = GetLastError();
        return result;
    }

    for (;;)
    {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            result.systemError = GetLastError();
            return result;
        }
        if (available == 0) break;

        const auto offset = result.body.size();
        result.body.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(request, result.body.data() + offset, available, &read))
        {
            result.systemError = GetLastError();
            return result;
        }
        result.body.resize(offset + read);
    }

    result.ok = result.status >= 200 && result.status < 300;
    return result;
}

bool httpDownload(const std::string& url, const std::filesystem::path& destination,
                  std::uint64_t& bytesWritten, DWORD& status, DWORD& systemError)
{
    bytesWritten = 0;
    status = 0;
    systemError = 0;

    std::wstring host, path;
    INTERNET_PORT port = 0;
    bool secure = false;
    if (!crackUrl(url, host, path, port, secure))
    {
        systemError = ERROR_INVALID_PARAMETER;
        return false;
    }

    WinHttpHandle session(WinHttpOpen(L"PMX-Updater/0.2",
                                      WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.valid()) { systemError = GetLastError(); return false; }
    WinHttpSetTimeouts(session, 10000, 10000, 10000, 30000);

    WinHttpHandle connection(WinHttpConnect(session, host.c_str(), port, 0));
    if (!connection.valid()) { systemError = GetLastError(); return false; }

    const DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    WinHttpHandle request(WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
                                             WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request.valid()) { systemError = GetLastError(); return false; }

    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(request, nullptr))
    {
        systemError = GetLastError();
        return false;
    }

    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(request,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &status, &statusSize, WINHTTP_NO_HEADER_INDEX))
    {
        systemError = GetLastError();
        return false;
    }
    if (status < 200 || status >= 300) return false;

    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output) return false;

    std::array<char, 64 * 1024> buffer {};
    for (;;)
    {
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), static_cast<DWORD>(buffer.size()), &read))
        {
            systemError = GetLastError();
            return false;
        }
        if (read == 0) break;
        output.write(buffer.data(), static_cast<std::streamsize>(read));
        if (!output) return false;
        bytesWritten += read;
    }
    output.flush();
    return static_cast<bool>(output);
}
#else
juce::URL::InputStreamOptions networkOptions(int* status)
{
    return juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000)
        .withNumRedirectsToFollow(5)
        .withStatusCode(status)
        .withExtraHeaders("Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\nUser-Agent: PMX-Updater\r\n");
}
#endif
}

std::vector<ReleaseInfo> JuceReleaseClient::fetchReleases(std::string& error)
{
#if defined(_WIN32)
    const auto response = httpGet("https://api.github.com/repos/KaranBarua01/PMX/releases?per_page=20");
    if (!response.ok)
    {
        if (response.status != 0)
            error = "GitHub returned HTTP " + std::to_string(response.status) + ".";
        else
            error = "Could not reach GitHub (Windows network error "
                + std::to_string(response.systemError) + ").";
        return {};
    }
    return ReleaseChecker::parseGitHubReleasesJson(response.body, error);
#else
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
#endif
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

#if defined(_WIN32)
        std::uint64_t written = 0;
        DWORD status = 0;
        DWORD systemError = 0;
        if (!httpDownload(release.assetUrl, partial, written, status, systemError))
        {
            std::filesystem::remove(partial, ec);
            if (status != 0)
                return {false, "Installer download failed with HTTP " + std::to_string(status) + ".", {}};
            return {false, "Installer download failed (Windows network error "
                + std::to_string(systemError) + ").", {}};
        }
#else
        int status = 0;
        auto stream = juce::URL(juce::String(release.assetUrl)).createInputStream(networkOptions(&status));
        if (!stream || status < 200 || status >= 300)
            return {false, "Installer download failed.", {}};

        juce::File file(juce::String(partial.string()));
        auto output = std::make_unique<juce::FileOutputStream>(file);
        if (output->failedToOpen()) return {false, "Downloaded installer could not be created.", {}};
        const auto streamWritten = output->writeFromInputStream(*stream, -1);
        output->flush();
        output.reset();
        if (streamWritten < 0)
        {
            std::filesystem::remove(partial, ec);
            return {false, "Installer download was incomplete.", {}};
        }
        const auto written = static_cast<std::uint64_t>(streamWritten);
#endif

        if (release.assetSize != 0 && written != release.assetSize)
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
