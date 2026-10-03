#include <cstdint>
#include "ui/PmxTheme.h"

int main()
{
    using pmx::ui::Theme;
    if (Theme::background != 0xFF0B0F14u) return 1;
    if (Theme::panel != 0xFF151A21u) return 2;
    if (Theme::accent != 0xFF3478F6u) return 3;
    if (Theme::healthy != 0xFF28C97Bu) return 4;
    if (Theme::danger != 0xFFE55263u) return 5;
    if (Theme::cornerRadius < 8.0f || Theme::cornerRadius > 16.0f) return 6;
    if (Theme::minimumWindowWidth > 1366 || Theme::minimumWindowHeight > 768) return 7;
    return 0;
}
