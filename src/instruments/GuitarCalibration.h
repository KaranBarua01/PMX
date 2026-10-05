#pragma once
#include <array>
#include "GuitarProfile.h"
#include "analysis/TunerEngine.h"

namespace pmx::instruments
{
class GuitarCalibration final
{
public:
    void start(const GuitarProfile& seed = GuitarProfile::standard()) noexcept;
    void cancel() noexcept;
    bool submit(const analysis::TunerResult& result) noexcept;

    [[nodiscard]] bool active() const noexcept { return isActive; }
    [[nodiscard]] bool completed() const noexcept { return isCompleted; }
    [[nodiscard]] int currentString() const noexcept { return isActive ? stringIndex : -1; }
    [[nodiscard]] const GuitarProfile& profile() const noexcept { return learnedProfile; }

private:
    static constexpr int readingsNeeded = 4;
    static constexpr float minimumConfidence = 0.45f;
    static constexpr double stableCents = 18.0;

    GuitarProfile learnedProfile;
    std::array<double, readingsNeeded> readings {};
    int readingCount {};
    int stringIndex {};
    bool isActive {};
    bool isCompleted {};
};
} // namespace pmx::instruments
