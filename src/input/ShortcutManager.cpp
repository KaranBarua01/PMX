#include "ShortcutManager.h"
#include <cctype>

namespace pmx::input
{
std::optional<ShortcutCommand> ShortcutManager::commandFor(int keyCode, const ShortcutContext& context) noexcept
{
    if (context.textEntryFocused || context.modalDialogOpen) return std::nullopt;
    if (keyCode == ' ') return ShortcutCommand::loopTransport;
    const auto key = std::tolower(static_cast<unsigned char>(keyCode));
    switch (key)
    {
        case 'b': return ShortcutCommand::bypass;
        case 'm': return ShortcutCommand::mute;
        case 't': return ShortcutCommand::tuner;
        case 's': return ShortcutCommand::stop;
        case 'p': return ShortcutCommand::tapTempo;
        case 'r': return ShortcutCommand::quickRecord;
        default: return std::nullopt;
    }
}
} // namespace pmx::input
