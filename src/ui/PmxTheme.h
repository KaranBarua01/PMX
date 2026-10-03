#pragma once
#include <cstdint>

namespace pmx::ui
{
struct Theme final
{
    static constexpr std::uint32_t background = 0xFF0B0F14u;
    static constexpr std::uint32_t topBar = 0xFF0A0E13u;
    static constexpr std::uint32_t panel = 0xFF151A21u;
    static constexpr std::uint32_t panelRaised = 0xFF1B212Bu;
    static constexpr std::uint32_t border = 0xFF252C36u;
    static constexpr std::uint32_t text = 0xFFF4F7FBu;
    static constexpr std::uint32_t mutedText = 0xFF8E99A8u;
    static constexpr std::uint32_t accent = 0xFF3478F6u;
    static constexpr std::uint32_t accentHover = 0xFF4A8BFFu;
    static constexpr std::uint32_t healthy = 0xFF28C97Bu;
    static constexpr std::uint32_t warning = 0xFFF2B84Bu;
    static constexpr std::uint32_t danger = 0xFFE55263u;

    static constexpr float cornerRadius = 10.0f;
    static constexpr int topBarHeight = 52;
    static constexpr int pagePadding = 24;
    static constexpr int cardGap = 12;
    static constexpr int minimumWindowWidth = 1100;
    static constexpr int minimumWindowHeight = 680;
};
} // namespace pmx::ui
