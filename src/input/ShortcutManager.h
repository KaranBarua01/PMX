#pragma once
#include <optional>

namespace pmx::input
{
enum class ShortcutCommand { bypass, mute, tuner, loopTransport, stop, tapTempo, quickRecord };
struct ShortcutContext { bool textEntryFocused{false}; bool modalDialogOpen{false}; };

class ShortcutManager final
{
public:
    static std::optional<ShortcutCommand> commandFor(int keyCode, const ShortcutContext&) noexcept;
};
} // namespace pmx::input
