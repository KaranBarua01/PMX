#include "presets/SoundState.h"
#include "presets/PresetStore.h"
#include <cmath>
#include <filesystem>
#include <limits>
int main()
{
    pmx::presets::SoundState sound;
    sound.effects[2].enabled=true;
    sound.effects[2].values={72,30,-3};
    sound.effects[5].values={-4,2,6};
    sound.effects[7].values={611,43,37};
    sound.nam.path="missing-amp.nam"; sound.ir.path="missing-cab.wav";
    const auto preset=sound.toPreset("test-tone","My Tone","Rock");
    if(preset.parameters.at("drive.amountPercent")!=72 || preset.parameters.at("eq.lowDb")!=-4) return 1;
    const auto dir=std::filesystem::temp_directory_path()/"pmx_sound_roundtrip";
    std::filesystem::create_directories(dir);
    pmx::presets::PresetStore store(dir);
    if(!store.save(preset).ok) return 2;
    const auto loaded=store.load("test-tone");
    if(!loaded.ok) return 3;
    const auto restored=pmx::presets::SoundState::fromPreset(loaded.preset);
    if(restored.effects[7].values[1]!=43 || restored.effects[2].values[2]!=-3 || !restored.effects[2].enabled) return 4;
    if(store.missingAssets(loaded.preset).size()!=2) return 5;
    auto invalid=preset;
    invalid.parameters["delay.feedbackPercent"]=999;
    invalid.parameters["drive.amountPercent"]=std::numeric_limits<double>::quiet_NaN();
    invalid.parameters["audio.muted"]=0;
    const auto safe=pmx::presets::SoundState::fromPreset(invalid);
    if(safe.effects[7].values[1]!=95 || !std::isfinite(safe.effects[2].values[0])) return 6;
    if(safe.toPreset("safe","Safe","Clean").parameters.contains("audio.muted")) return 7;
    std::filesystem::remove_all(dir);
    return 0;
}
