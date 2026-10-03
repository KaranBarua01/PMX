#pragma once
#include <array>
#include <string_view>
#include "effects/Gate.h"
#include "effects/Compressor.h"
#include "effects/Drive.h"
#include "effects/Eq.h"
#include "effects/Chorus.h"
#include "effects/Delay.h"
#include "effects/Reverb.h"

namespace pmx::dsp
{
enum class RackModule : unsigned { gate, comp, drive, nam, ir, eq, mod, delay, reverb, count };

class GuitarRack final
{
public:
    static constexpr std::array<std::string_view, 9> moduleNames() noexcept { return {"Gate","Comp","Drive","NAM","IR","EQ","Mod","Delay","Reverb"}; }
    void prepare(double sampleRate, int maxBlockSize);
    void setEnabled(RackModule module, bool enabled) noexcept { active[static_cast<unsigned>(module)] = enabled; }
    [[nodiscard]] bool isEnabled(RackModule module) const noexcept { return active[static_cast<unsigned>(module)]; }
    void setDriveAmount(float v) noexcept { drive.setAmount(v); }
    Delay& delayEffect() noexcept { return delay; }
    Chorus& chorusEffect() noexcept { return chorus; }
    Reverb& reverbEffect() noexcept { return reverb; }
    Eq& eqEffect() noexcept { return eq; }
    Gate& gateEffect() noexcept { return gate; }
    Compressor& compressorEffect() noexcept { return comp; }
    void processPreModels(float* mono, int numSamples) noexcept;
    void processPostModels(float* mono, int numSamples) noexcept;
    void process(float* mono, int numSamples) noexcept;
private:
    std::array<bool, static_cast<unsigned>(RackModule::count)> active{};
    Gate gate; Compressor comp; Drive drive; Eq eq; Chorus chorus; Delay delay; Reverb reverb;
};
} // namespace pmx::dsp
