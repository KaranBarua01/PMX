#include "input/ShortcutManager.h"

int main()
{
    using namespace pmx::input;
    ShortcutContext normal{};
    if(ShortcutManager::commandFor('b',normal)!=ShortcutCommand::bypass) return 1;
    if(ShortcutManager::commandFor('M',normal)!=ShortcutCommand::mute) return 2;
    if(ShortcutManager::commandFor('t',normal)!=ShortcutCommand::tuner) return 3;
    if(ShortcutManager::commandFor(' ',normal)!=ShortcutCommand::loopTransport) return 4;
    if(ShortcutManager::commandFor('s',normal)!=ShortcutCommand::stop) return 5;
    if(ShortcutManager::commandFor('p',normal)!=ShortcutCommand::tapTempo) return 6;
    if(ShortcutManager::commandFor('r',normal)!=ShortcutCommand::quickRecord) return 7;
    if(ShortcutManager::commandFor('x',normal).has_value()) return 8;

    ShortcutContext typing; typing.textEntryFocused=true;
    if(ShortcutManager::commandFor('b',typing).has_value()) return 9;
    if(ShortcutManager::commandFor(' ',typing).has_value()) return 10;

    ShortcutContext modal; modal.modalDialogOpen=true;
    if(ShortcutManager::commandFor('r',modal).has_value()) return 11;
    return 0;
}
