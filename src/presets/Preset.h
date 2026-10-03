#pragma once
#include <filesystem>
#include <map>
#include <string>

namespace pmx::presets
{
enum class PresetMode { guitar, bass, synth, drums, piano, violin };

struct AssetReference
{
    std::string id;
    std::filesystem::path path;
};

struct Preset
{
    int schemaVersion { 1 };
    std::string id;
    std::string displayName;
    std::string category { "User" };
    PresetMode mode { PresetMode::guitar };
    bool favorite { false };
    std::map<std::string, double> parameters;
    std::map<std::string, bool> modules;
    AssetReference nam;
    AssetReference ir;
};
} // namespace pmx::presets
