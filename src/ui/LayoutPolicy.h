#pragma once
#include <algorithm>
#include "PmxTheme.h"

namespace pmx::ui
{
struct LayoutMetrics
{
    bool supported { false };
    int contentWidth { 0 };
    int contentHeight { 0 };
    int dialogWidth { 0 };
    int dialogHeight { 0 };
    int presetCardWidth { 0 };
};

struct LayoutPolicy final
{
    static constexpr LayoutMetrics compute(int windowWidth, int windowHeight) noexcept
    {
        LayoutMetrics m;
        m.supported = windowWidth >= Theme::minimumWindowWidth && windowHeight >= Theme::minimumWindowHeight;
        m.contentWidth = std::max(0, windowWidth - Theme::pagePadding * 2);
        m.contentHeight = std::max(0, windowHeight - Theme::topBarHeight - Theme::pagePadding * 2);
        m.dialogWidth = std::max(0, std::min(560, windowWidth - 80));
        m.dialogHeight = std::max(0, std::min(470, windowHeight - 80));
        const int presetBody = std::max(0, m.contentWidth - 180 - 18);
        m.presetCardWidth = std::max(0, (presetBody - Theme::cardGap * 3) / 4);
        return m;
    }
};
} // namespace pmx::ui
