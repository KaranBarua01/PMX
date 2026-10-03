#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include "presets/PresetStore.h"

namespace fs = std::filesystem;

static pmx::presets::Preset makePreset(const fs::path& assetDir)
{
    pmx::presets::Preset p;
    p.schemaVersion = 1;
    p.id = "dream-clean";
    p.displayName = "Dream Clean";
    p.category = "Clean";
    p.mode = pmx::presets::PresetMode::guitar;
    p.favorite = true;
    p.parameters["delay.timeMs"] = 420.0;
    p.parameters["delay.mixPercent"] = 24.0;
    p.modules["delay"] = true;
    p.modules["drive"] = false;
    p.nam = {"nam-be100", assetDir / "BE100-BE.nam"};
    p.ir = {"ir-24sve", assetDir / "24sVe 2x12.wav"};
    return p;
}

int main()
{
    const auto root = fs::temp_directory_path() / "pmx_preset_store_test";
    fs::remove_all(root);
    fs::create_directories(root / "assets");
    { std::ofstream(root / "assets" / "BE100-BE.nam") << "{}"; }
    { std::ofstream(root / "assets" / "24sVe 2x12.wav") << "RIFF"; }

    pmx::presets::PresetStore store(root / "presets");
    const auto original = makePreset(root / "assets");

    const auto firstSave = store.save(original);
    if (!firstSave.ok) return 1;
    if (!fs::exists(firstSave.path)) return 2;
    if (fs::exists(firstSave.path.string() + ".tmp")) return 3;

    const auto loaded = store.load(original.id);
    if (!loaded.ok) return 4;
    if (loaded.preset.schemaVersion != 1) return 5;
    if (loaded.preset.displayName != "Dream Clean") return 6;
    if (loaded.preset.category != "Clean") return 7;
    if (loaded.preset.mode != pmx::presets::PresetMode::guitar) return 8;
    if (!loaded.preset.favorite) return 9;
    if (std::abs(loaded.preset.parameters.at("delay.timeMs") - 420.0) > 0.001) return 10;
    if (!loaded.preset.modules.at("delay")) return 11;
    if (loaded.preset.modules.at("drive")) return 12;
    if (loaded.preset.nam.id != "nam-be100") return 13;
    if (loaded.preset.ir.id != "ir-24sve") return 14;

    const auto missingBefore = store.missingAssets(loaded.preset);
    if (!missingBefore.empty()) return 15;
    fs::remove(root / "assets" / "BE100-BE.nam");
    const auto missingAfter = store.missingAssets(loaded.preset);
    if (missingAfter.size() != 1 || missingAfter.front() != "NAM") return 16;

    auto edited = loaded.preset;
    edited.parameters["delay.timeMs"] = 360.0;
    store.setCurrent(loaded.preset);
    if (store.isDirty()) return 17;
    store.setCurrentParameter("delay.timeMs", 360.0);
    if (!store.isDirty()) return 18;

    const auto secondSave = store.save(edited);
    if (!secondSave.ok) return 19;
    if (!fs::exists(secondSave.path.string() + ".bak")) return 20;
    const auto loadedEdited = store.load(original.id);
    if (!loadedEdited.ok || std::abs(loadedEdited.preset.parameters.at("delay.timeMs") - 360.0) > 0.001) return 21;
    if (fs::exists(secondSave.path.string() + ".tmp")) return 22;

    const auto all = store.list();
    if (all.size() != 1 || all.front().id != original.id) return 23;

    fs::remove_all(root);
    return 0;
}
