#include "ReleaseChecker.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace pmx::update
{
namespace
{
bool numeric(const std::string& s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c); });
}
std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> out; std::string item; std::istringstream in(s);
    while(std::getline(in,item,delimiter)) out.push_back(item);
    return out;
}


std::optional<std::string> jsonStringField(const std::string& object, const std::string& key)
{
    const auto marker = "\"" + key + "\"";
    auto p = object.find(marker);
    if (p == std::string::npos) return std::nullopt;
    p = object.find(':', p + marker.size());
    if (p == std::string::npos) return std::nullopt;
    p = object.find('"', p + 1);
    if (p == std::string::npos) return std::nullopt;
    std::string out; bool escaped=false;
    for(++p;p<object.size();++p)
    {
        const char c=object[p];
        if(escaped)
        {
            switch(c){case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;case '"':out+='"';break;case '\\':out+='\\';break;default:out+=c;break;}
            escaped=false;
        }
        else if(c=='\\') escaped=true;
        else if(c=='"') return out;
        else out+=c;
    }
    return std::nullopt;
}

std::optional<bool> jsonBoolField(const std::string& object,const std::string& key)
{
    const auto marker="\""+key+"\""; auto p=object.find(marker); if(p==std::string::npos)return std::nullopt;
    p=object.find(':',p+marker.size()); if(p==std::string::npos)return std::nullopt;
    ++p; while(p<object.size()&&std::isspace(static_cast<unsigned char>(object[p])))++p;
    if(object.compare(p,4,"true")==0)return true; if(object.compare(p,5,"false")==0)return false; return std::nullopt;
}

std::optional<std::uint64_t> jsonUIntField(const std::string& object,const std::string& key)
{
    const auto marker="\""+key+"\""; auto p=object.find(marker); if(p==std::string::npos)return std::nullopt;
    p=object.find(':',p+marker.size()); if(p==std::string::npos)return std::nullopt;
    ++p; while(p<object.size()&&std::isspace(static_cast<unsigned char>(object[p])))++p;
    auto e=p; while(e<object.size()&&std::isdigit(static_cast<unsigned char>(object[e])))++e;
    if(e==p)return std::nullopt; try{return static_cast<std::uint64_t>(std::stoull(object.substr(p,e-p)));}catch(...){return std::nullopt;}
}

std::vector<std::string> objectList(const std::string& text)
{
    std::vector<std::string> out; int depth=0; std::size_t start=0; bool inString=false,escaped=false;
    for(std::size_t i=0;i<text.size();++i)
    {
        const char c=text[i];
        if(inString){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')inString=false;continue;}
        if(c=='"'){inString=true;continue;}
        if(c=='{'){if(depth==0)start=i;++depth;}
        else if(c=='}'&&depth>0){--depth;if(depth==0)out.push_back(text.substr(start,i-start+1));}
    }
    return out;
}

std::optional<std::string> arrayField(const std::string& object,const std::string& key)
{
    const auto marker="\""+key+"\""; auto p=object.find(marker); if(p==std::string::npos)return std::nullopt;
    p=object.find('[',p+marker.size()); if(p==std::string::npos)return std::nullopt;
    int depth=0; bool inString=false,escaped=false;
    for(std::size_t i=p;i<object.size();++i)
    {
        const char c=object[i];
        if(inString){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')inString=false;continue;}
        if(c=='"'){inString=true;continue;}
        if(c=='[')++depth; else if(c==']'&&--depth==0)return object.substr(p+1,i-p-1);
    }
    return std::nullopt;
}

bool endsWithCaseInsensitive(std::string value,std::string suffix)
{
    if(value.size()<suffix.size())return false;
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    std::transform(suffix.begin(),suffix.end(),suffix.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return value.compare(value.size()-suffix.size(),suffix.size(),suffix)==0;
}

class Sha256
{
public:
    void update(const std::uint8_t* data, std::size_t len)
    {
        for(std::size_t i=0;i<len;++i)
        {
            buffer[bufferLen++] = data[i];
            ++totalBytes;
            if(bufferLen==64){transform();bufferLen=0;}
        }
    }
    std::string finish()
    {
        const auto bits=totalBytes*8ull;
        buffer[bufferLen++]=0x80;
        if(bufferLen>56){while(bufferLen<64)buffer[bufferLen++]=0;transform();bufferLen=0;}
        while(bufferLen<56)buffer[bufferLen++]=0;
        for(int i=7;i>=0;--i)buffer[bufferLen++]=static_cast<std::uint8_t>((bits>>(i*8))&0xff);
        transform();
        std::ostringstream out; out<<std::hex<<std::setfill('0');
        for(auto v:state) out<<std::setw(8)<<v;
        return out.str();
    }
private:
    static constexpr std::array<std::uint32_t,64> k {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
    static std::uint32_t rotr(std::uint32_t v,int n){return (v>>n)|(v<<(32-n));}
    void transform()
    {
        std::uint32_t w[64]{};
        for(int i=0;i<16;++i) w[i]=(static_cast<std::uint32_t>(buffer[i*4])<<24)|(static_cast<std::uint32_t>(buffer[i*4+1])<<16)|(static_cast<std::uint32_t>(buffer[i*4+2])<<8)|buffer[i*4+3];
        for(int i=16;i<64;++i){auto s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3);auto s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
        auto a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
        for(int i=0;i<64;++i){auto s1=rotr(e,6)^rotr(e,11)^rotr(e,25);auto ch=(e&f)^((~e)&g);auto t1=h+s1+ch+k[i]+w[i];auto s0=rotr(a,2)^rotr(a,13)^rotr(a,22);auto maj=(a&b)^(a&c)^(b&c);auto t2=s0+maj;h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
        state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=h;
    }
    std::array<std::uint32_t,8> state{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::array<std::uint8_t,64> buffer{}; std::size_t bufferLen{0}; std::uint64_t totalBytes{0};
};
}

std::optional<SemanticVersion> SemanticVersion::parse(std::string text)
{
    if(!text.empty() && (text.front()=='v'||text.front()=='V')) text.erase(text.begin());
    const auto plus=text.find('+'); if(plus!=std::string::npos) text=text.substr(0,plus);
    std::string pre; const auto dash=text.find('-'); if(dash!=std::string::npos){pre=text.substr(dash+1);text=text.substr(0,dash);}
    auto core=split(text,'.'); if(core.size()!=3||!numeric(core[0])||!numeric(core[1])||!numeric(core[2])) return std::nullopt;
    SemanticVersion v; try{v.major=std::stoi(core[0]);v.minor=std::stoi(core[1]);v.patch=std::stoi(core[2]);}catch(...){return std::nullopt;}
    if(!pre.empty()) v.prerelease=split(pre,'.');
    return v;
}

bool operator<(const SemanticVersion& a,const SemanticVersion& b)
{
    if(a.major!=b.major)return a.major<b.major; if(a.minor!=b.minor)return a.minor<b.minor; if(a.patch!=b.patch)return a.patch<b.patch;
    if(a.prerelease.empty()!=b.prerelease.empty()) return !b.prerelease.empty();
    for(std::size_t i=0;i<std::min(a.prerelease.size(),b.prerelease.size());++i)
    {
        if(a.prerelease[i]==b.prerelease[i])continue;
        const bool an=numeric(a.prerelease[i]),bn=numeric(b.prerelease[i]);
        if(an&&bn)return std::stoll(a.prerelease[i])<std::stoll(b.prerelease[i]);
        if(an!=bn)return an;
        return a.prerelease[i]<b.prerelease[i];
    }
    return a.prerelease.size()<b.prerelease.size();
}

UpdateDecision ReleaseChecker::evaluate(const SemanticVersion& current,UpdateChannel channel,const std::vector<ReleaseInfo>& releases)
{
    const ReleaseInfo* best=nullptr; std::optional<SemanticVersion> bestVersion;
    for(const auto& release:releases)
    {
        if(release.draft || (channel==UpdateChannel::stable && release.prerelease)) continue;
        auto version=SemanticVersion::parse(release.tagName); if(!version) continue;
        if(!bestVersion || *bestVersion < *version){best=&release;bestVersion=*version;}
    }
    if(!best || !bestVersion || ! (current < *bestVersion)) return {UpdateStatus::upToDate,"PMX is up to date.",{}};
    return {UpdateStatus::available,"A PMX update is available.",*best};
}

UpdateDecision ReleaseChecker::offline(std::string message){return {UpdateStatus::error,std::move(message),{}};}

std::future<UpdateDecision> ReleaseChecker::checkAsync(SemanticVersion current,UpdateChannel channel,Provider provider)
{
    return std::async(std::launch::async,[current=std::move(current),channel,provider=std::move(provider)]() mutable {
        std::string error; auto releases=provider(error); if(!error.empty())return offline(error); return evaluate(current,channel,releases);
    });
}

bool ReleaseChecker::verifySha256(const std::filesystem::path& path,const std::string& expectedDigest,std::string& error)
{
    constexpr std::string_view prefix="sha256:";
    if(expectedDigest.rfind(prefix,0)!=0 || expectedDigest.size()!=prefix.size()+64){error="Release is missing a valid SHA-256 digest.";return false;}
    std::ifstream in(path,std::ios::binary); if(!in){error="Downloaded installer could not be opened for SHA-256 verification.";return false;}
    Sha256 sha; std::array<std::uint8_t,32768> bytes{};
    while(in){in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));const auto got=in.gcount();if(got>0)sha.update(bytes.data(),static_cast<std::size_t>(got));}
    const auto actual=sha.finish(); std::string expected=expectedDigest.substr(prefix.size());
    std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    if(actual!=expected){error="SHA-256 digest mismatch; the installer will not be opened.";return false;}
    error.clear(); return true;
}

std::vector<ReleaseInfo> ReleaseChecker::parseGitHubReleasesJson(const std::string& json,std::string& error)
{
    std::vector<ReleaseInfo> releases;
    if(json.empty()){error="GitHub returned an empty release response.";return releases;}
    for(const auto& object:objectList(json))
    {
        const auto tag=jsonStringField(object,"tag_name");
        const auto name=jsonStringField(object,"name");
        const auto draft=jsonBoolField(object,"draft");
        const auto prerelease=jsonBoolField(object,"prerelease");
        if(!tag||!draft||!prerelease)continue;
        ReleaseInfo release; release.tagName=*tag; release.name=name.value_or(*tag); release.draft=*draft; release.prerelease=*prerelease; release.notes=jsonStringField(object,"body").value_or("");
        if(const auto assets=arrayField(object,"assets"))
            for(const auto& asset:objectList(*assets))
            {
                const auto assetName=jsonStringField(asset,"name"); if(!assetName||!endsWithCaseInsensitive(*assetName,".exe"))continue;
                const auto url=jsonStringField(asset,"browser_download_url"); const auto size=jsonUIntField(asset,"size"); const auto digest=jsonStringField(asset,"digest");
                if(!url||!size||!digest)continue;
                release.assetName=*assetName; release.assetUrl=*url; release.assetSize=*size; release.digest=*digest; break;
            }
        releases.push_back(std::move(release));
    }
    if(releases.empty())error="GitHub release information could not be read.";else error.clear();
    return releases;
}

bool ReleaseChecker::isTrustedInstallerUrl(const std::string& url)
{
    constexpr std::string_view prefix="https://github.com/KaranBarua01/PMX/releases/download/";
    return url.rfind(prefix,0)==0 && endsWithCaseInsensitive(url,".exe");
}

} // namespace pmx::update
