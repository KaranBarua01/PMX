#include <cmath>
#include <filesystem>
#include "instruments/GuitarCalibration.h"
#include "instruments/GuitarProfile.h"

namespace
{
bool near(double a, double b, double tolerance)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using pmx::instruments::GuitarCalibration;
    using pmx::instruments::GuitarProfile;

    auto profile = GuitarProfile::standard();
    if (!profile.valid()) return 1;
    if (!near(profile.openString(0), 82.4069, 0.001)) return 2;
    if (!near(profile.fretFrequency(0, 12), 164.8138, 0.02)) return 3;
    if (!near(profile.fretFrequency(5, 24), 1318.512, 0.08)) return 4;
    if (profile.fretFrequency(0, 25) != 0.0) return 5;

    const auto path = std::filesystem::temp_directory_path() / "pmx-guitar-profile-test.txt";
    std::error_code error;
    std::filesystem::remove(path, error);
    if (!profile.setOpenString(0, 73.4162)) return 6;
    if (!profile.save(path)) return 7;

    bool loaded = false;
    const auto restored = GuitarProfile::loadOrStandard(path, &loaded);
    std::filesystem::remove(path, error);
    if (!loaded || !near(restored.openString(0), 73.4162, 0.0001)) return 8;

    GuitarCalibration calibration;
    calibration.start(GuitarProfile::standard());
    constexpr double semitoneDownTwo = 0.8908987181403393;
    constexpr double standard[6] { 82.4069, 110.0, 146.832, 195.998, 246.942, 329.628 };

    for (int stringIndex = 0; stringIndex < 6; ++stringIndex)
    {
        const auto target = standard[stringIndex] * semitoneDownTwo;
        for (int reading = 0; reading < 4; ++reading)
        {
            pmx::analysis::TunerResult result;
            result.frequencyHz = target * (1.0 + (reading - 1.5) * 0.0002);
            result.confidence = 0.92f;
            const auto advanced = calibration.submit(result);
            if ((reading == 3) != advanced) return 20 + stringIndex;
        }
    }

    if (calibration.active() || !calibration.completed()) return 30;
    for (int stringIndex = 0; stringIndex < 6; ++stringIndex)
        if (!near(calibration.profile().openString(static_cast<std::size_t>(stringIndex)),
                  standard[stringIndex] * semitoneDownTwo, 0.08))
            return 31 + stringIndex;

    return 0;
}
