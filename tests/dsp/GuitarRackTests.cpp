#include <array>
#include <string_view>
#include "dsp/GuitarRack.h"

int main()
{
    using pmx::dsp::GuitarRack;
    using pmx::dsp::RackModule;
    constexpr std::array<std::string_view, 9> expected {"Gate","Comp","Drive","NAM","IR","EQ","Mod","Delay","Reverb"};
    if (GuitarRack::moduleNames() != expected) return 1;

    GuitarRack rack;
    rack.prepare(48000.0, 128);
    rack.setEnabled(RackModule::drive, true);
    if (! rack.isEnabled(RackModule::drive)) return 2;
    rack.setDriveAmount(0.8f);
    float buffer[] {0.1f, 0.3f, -0.4f};
    rack.process(buffer, 3);
    if (buffer[1] == 0.3f) return 3;

    rack.setEnabled(RackModule::drive, false);
    if (rack.isEnabled(RackModule::drive)) return 4;
    return 0;
}
