#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace pmx::assets
{
enum class AssetKind { nam, ir };
struct AssetRecord { std::string id; AssetKind kind{AssetKind::nam}; std::string displayName; std::filesystem::path path; bool favorite{false}; };
struct ImportResult { bool ok{false}; std::string error; AssetRecord asset; };

class AssetLibrary final
{
public:
    ImportResult importFile(const std::filesystem::path&, AssetKind);
    [[nodiscard]] std::vector<AssetRecord> assets(AssetKind) const;
    bool remove(const std::string& id);
    bool setFavorite(const std::string& id, bool favorite);
private:
    std::vector<AssetRecord> items;
};
} // namespace pmx::assets
