#pragma once
#include <array>
#include <cstddef>
#include <filesystem>

namespace pmx::instruments
{
class GuitarProfile final
{
public:
    static constexpr std::size_t stringCount = 6;
    static constexpr int maxFret = 24;

    GuitarProfile() noexcept = default;
    explicit GuitarProfile(const std::array<double, stringCount>& frequencies) noexcept;

    [[nodiscard]] static GuitarProfile standard() noexcept;
    [[nodiscard]] const std::array<double, stringCount>& openStrings() const noexcept { return openStringHz; }
    [[nodiscard]] double openString(std::size_t index) const noexcept;
    [[nodiscard]] double fretFrequency(std::size_t stringIndex, int fret) const noexcept;
    [[nodiscard]] bool valid() const noexcept;

    bool setOpenString(std::size_t index, double frequencyHz) noexcept;
    [[nodiscard]] bool save(const std::filesystem::path& path) const;
    [[nodiscard]] static GuitarProfile loadOrStandard(const std::filesystem::path& path, bool* loaded = nullptr);

private:
    std::array<double, stringCount> openStringHz { 82.4069, 110.0, 146.832, 195.998, 246.942, 329.628 };
};
} // namespace pmx::instruments
