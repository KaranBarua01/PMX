#include <filesystem>
#include <fstream>
#include "update/ReleaseChecker.h"

int main()
{
    using namespace pmx::update;

    const auto current=SemanticVersion::parse("0.2.0-alpha.1");
    if(!current) return 1;
    std::vector<ReleaseInfo> releases {
        {"0.2.0-alpha.2","Preview 2",true,false,"changes","PMX-0.2.0-alpha.2-x64.exe","https://example.invalid/a.exe",123,"sha256:deadbeef"},
        {"0.1.9","Old stable",false,false,"old","PMX-0.1.9-x64.exe","https://example.invalid/old.exe",100,"sha256:old"}
    };
    const auto preview=ReleaseChecker::evaluate(*current,UpdateChannel::preview,releases);
    if(preview.status!=UpdateStatus::available || preview.release.tagName!="0.2.0-alpha.2") return 2;

    const auto asyncDecision=ReleaseChecker::checkAsync(*current,UpdateChannel::preview,[releases](std::string&){return releases;}).get();
    if(asyncDecision.status!=UpdateStatus::available) return 10;

    const auto stableCurrent=SemanticVersion::parse("1.0.0");
    if(!stableCurrent) return 3;
    releases={
        {"1.1.0-beta.1","Beta",true,false,"beta","beta.exe","https://example.invalid/beta",1,"sha256:x"},
        {"1.0.1","Stable",false,false,"stable","PMX-1.0.1-x64.exe","https://example.invalid/stable",1,"sha256:y"}
    };
    const auto stable=ReleaseChecker::evaluate(*stableCurrent,UpdateChannel::stable,releases);
    if(stable.status!=UpdateStatus::available || stable.release.tagName!="1.0.1") return 4;

    const auto same=ReleaseChecker::evaluate(*SemanticVersion::parse("1.0.1"),UpdateChannel::stable,releases);
    if(same.status!=UpdateStatus::upToDate) return 5;

    const auto offline=ReleaseChecker::offline("No internet connection");
    if(offline.status!=UpdateStatus::error || offline.message.find("No internet") == std::string::npos) return 6;

    const auto file=std::filesystem::temp_directory_path()/"pmx_digest_test.txt";
    {std::ofstream out(file,std::ios::binary);out<<"abc";}
    std::string error;
    if(!ReleaseChecker::verifySha256(file,"sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",error)) return 7;
    if(ReleaseChecker::verifySha256(file,"sha256:0000000000000000000000000000000000000000000000000000000000000000",error)) return 8;
    if(error.find("digest") == std::string::npos && error.find("SHA-256") == std::string::npos) return 9;
    std::filesystem::remove(file);

    const std::string apiJson = R"json([
      {"tag_name":"v1.2.0","name":"PMX 1.2","draft":false,"prerelease":false,"body":"Better audio", "assets":[
        {"name":"notes.txt","browser_download_url":"https://github.com/KaranBarua01/PMX/releases/download/v1.2.0/notes.txt","size":2,"digest":"sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
        {"name":"PMX-1.2.0-x64.exe","browser_download_url":"https://github.com/KaranBarua01/PMX/releases/download/v1.2.0/PMX-1.2.0-x64.exe","size":42,"digest":"sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"}
      ]}
    ])json";
    std::string parseError;
    const auto parsed = ReleaseChecker::parseGitHubReleasesJson(apiJson, parseError);
    if(!parseError.empty() || parsed.size()!=1 || parsed[0].tagName!="v1.2.0" || parsed[0].assetName!="PMX-1.2.0-x64.exe" || parsed[0].assetSize!=42) return 11;
    if(!ReleaseChecker::isTrustedInstallerUrl(parsed[0].assetUrl)) return 12;
    if(ReleaseChecker::isTrustedInstallerUrl("http://github.com/KaranBarua01/PMX/releases/download/v1/x.exe")) return 13;
    if(ReleaseChecker::isTrustedInstallerUrl("https://github.com/other/PMX/releases/download/v1/x.exe")) return 14;
    return 0;
}
