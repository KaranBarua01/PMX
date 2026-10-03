#include <filesystem>
#include <fstream>
#include "assets/AssetLibrary.h"

int main()
{
    namespace fs = std::filesystem;
    const auto dir = fs::temp_directory_path() / "pmx_asset_test";
    fs::create_directories(dir);
    const auto nam = dir / "BE100-BE.nam";
    { std::ofstream f(nam); f << "{}"; }

    pmx::assets::AssetLibrary library;
    auto imported = library.importFile(nam, pmx::assets::AssetKind::nam);
    if (! imported.ok) return 1;
    if (imported.asset.displayName != "BE100-BE") return 2;
    if (imported.asset.id.empty()) return 3;
    if (library.assets(pmx::assets::AssetKind::nam).size() != 1) return 4;

    const auto bad = dir / "bad.txt";
    { std::ofstream f(bad); f << "x"; }
    if (library.importFile(bad, pmx::assets::AssetKind::nam).ok) return 5;
    if (library.importFile(dir / "missing.nam", pmx::assets::AssetKind::nam).ok) return 6;
    fs::remove_all(dir);
    return 0;
}
