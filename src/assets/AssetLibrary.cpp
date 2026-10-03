#include "AssetLibrary.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>

namespace pmx::assets
{
namespace
{
std::string lower(std::string s){ for(auto&c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c))); return s; }
std::string stableId(const std::filesystem::path& p, AssetKind k)
{
    const auto text = p.lexically_normal().generic_string() + (k==AssetKind::nam?"|nam":"|ir");
    std::uint64_t h=1469598103934665603ull; for(unsigned char c:text){h^=c;h*=1099511628211ull;}
    std::ostringstream os; os<<std::hex<<h; return os.str();
}
}
ImportResult AssetLibrary::importFile(const std::filesystem::path& path, AssetKind kind)
{
    if(!std::filesystem::exists(path)||!std::filesystem::is_regular_file(path)) return {false,"File does not exist.",{}};
    const auto ext=lower(path.extension().string());
    if((kind==AssetKind::nam&&ext!=".nam")||(kind==AssetKind::ir&&ext!=".wav")) return {false,kind==AssetKind::nam?"Choose a .nam model file.":"Choose a .wav IR file.",{}};
    AssetRecord rec{stableId(path,kind),kind,path.stem().string(),std::filesystem::absolute(path),false};
    auto it=std::find_if(items.begin(),items.end(),[&](const auto&i){return i.id==rec.id;});
    if(it==items.end()) items.push_back(rec); else rec=*it;
    return {true,{},rec};
}
std::vector<AssetRecord> AssetLibrary::assets(AssetKind kind) const { std::vector<AssetRecord> out; for(const auto&i:items) if(i.kind==kind) out.push_back(i); return out; }
bool AssetLibrary::remove(const std::string& id){auto old=items.size();items.erase(std::remove_if(items.begin(),items.end(),[&](const auto&i){return i.id==id;}),items.end());return items.size()!=old;}
bool AssetLibrary::setFavorite(const std::string&id,bool f){for(auto&i:items)if(i.id==id){i.favorite=f;return true;}return false;}
}
