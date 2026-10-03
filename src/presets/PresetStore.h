#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "Preset.h"

namespace pmx::presets
{
struct PresetSaveResult
{
    bool ok { false };
    std::string error;
    std::filesystem::path path;
};

struct PresetLoadResult
{
    bool ok { false };
    std::string error;
    Preset preset;
};

class PresetStore final
{
public:
    explicit PresetStore(std::filesystem::path directory);

    [[nodiscard]] PresetSaveResult save(const Preset& preset);
    [[nodiscard]] PresetLoadResult load(const std::string& id) const;
    [[nodiscard]] std::vector<Preset> list() const;
    [[nodiscard]] std::vector<std::string> missingAssets(const Preset& preset) const;

    void setCurrent(const Preset& preset);
    void setCurrentParameter(const std::string& key, double value);
    [[nodiscard]] bool isDirty() const noexcept { return dirty; }
    [[nodiscard]] const std::optional<Preset>& current() const noexcept { return currentPreset; }

private:
    std::filesystem::path presetPath(const std::string& id) const;
    std::filesystem::path directory;
    std::optional<Preset> currentPreset;
    bool dirty { false };
};
} // namespace pmx::presets
