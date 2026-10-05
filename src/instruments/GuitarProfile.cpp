#include "GuitarProfile.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>

namespace pmx::instruments
{
namespace
{
constexpr double minimumFrequency = 40.0;
constexpr double maximumFrequency = 1400.0;
constexpr const char* profileHeader = "PMX_GUITAR_PROFILE_V1";

bool validFrequency(double frequency) noexcept
{
    return std::isfinite(frequency) && frequency >= minimumFrequency && frequency <= maximumFrequency;
}
}

GuitarProfile::GuitarProfile(const std::array<double, stringCount>& frequencies) noexcept
{
    for (std::size_t i = 0; i < stringCount; ++i)
        if (validFrequency(frequencies[i]))
            openStringHz[i] = frequencies[i];
}

GuitarProfile GuitarProfile::standard() noexcept
{
    return GuitarProfile {};
}

double GuitarProfile::openString(std::size_t index) const noexcept
{
    return index < stringCount ? openStringHz[index] : 0.0;
}

double GuitarProfile::fretFrequency(std::size_t stringIndex, int fret) const noexcept
{
    if (stringIndex >= stringCount || fret < 0 || fret > maxFret) return 0.0;
    return openStringHz[stringIndex] * std::pow(2.0, static_cast<double>(fret) / 12.0);
}

bool GuitarProfile::valid() const noexcept
{
    for (const auto frequency : openStringHz)
        if (!validFrequency(frequency)) return false;
    return true;
}

bool GuitarProfile::setOpenString(std::size_t index, double frequencyHz) noexcept
{
    if (index >= stringCount || !validFrequency(frequencyHz)) return false;
    openStringHz[index] = frequencyHz;
    return true;
}

bool GuitarProfile::save(const std::filesystem::path& path) const
{
    if (!valid()) return false;
    std::error_code error;
    if (const auto parent = path.parent_path(); !parent.empty())
        std::filesystem::create_directories(parent, error);
    if (error) return false;

    std::ofstream stream(path, std::ios::trunc);
    if (!stream) return false;
    stream << profileHeader << '\n' << std::setprecision(17);
    for (const auto frequency : openStringHz)
        stream << frequency << '\n';
    return static_cast<bool>(stream);
}

GuitarProfile GuitarProfile::loadOrStandard(const std::filesystem::path& path, bool* loaded)
{
    if (loaded) *loaded = false;
    std::ifstream stream(path);
    if (!stream) return standard();

    std::string header;
    if (!std::getline(stream, header) || header != profileHeader) return standard();

    std::array<double, stringCount> frequencies {};
    for (auto& frequency : frequencies)
        if (!(stream >> frequency) || !validFrequency(frequency))
            return standard();

    GuitarProfile profile(frequencies);
    if (!profile.valid()) return standard();
    if (loaded) *loaded = true;
    return profile;
}
} // namespace pmx::instruments
