# PMX UI reference fidelity

The user-supplied collage at `docs/ui/pmx-reference.png` is the visual source of truth for the first PMX alpha. The implementation should follow its hierarchy and density rather than inventing a separate desktop-audio aesthetic.

## Shared visual language

- Near-black graphite canvas and top navigation.
- Charcoal cards with restrained one-pixel borders and roughly 8–12 px corner radii.
- Electric blue for selected/primary actions.
- Green only for connected/healthy states.
- Red only for recording, destructive actions, clipping, or faults.
- White primary text; subdued cool-grey secondary copy.
- Large whitespace between functional groups; compact labels inside technical groups.
- Advanced engineering detail is subordinate to the primary musical action.

## Reference panel mapping

1. **Welcome** — centered setup card, three-step progress, one primary Continue action.
2. **Pocket Master Found** — input/output/ASIO health cards and Test Audio.
3. **You're Ready** — success state, starter sound, Start Playing.
4. **Live** — large sound name, horizontal signal chain, NAM/IR cards, status strip, Tuner/Bypass/Save actions.
5. **Delay editor** — centered dark modal, three large rotary controls, Tap Tempo, Done.
6. **NAM/IR browser** — two-column amp/cabinet selector with a single Load Selected Pair action.
7. **Looper** — large waveform/progress area and four oversized transport tiles.
8. **Presets** — category rail, search/filter row, four-column sound-card grid.
9. **Settings** — connection/stability controls on the left, latency/help status on the right.
10. **Update** — small centered release dialog with What's New and one primary Update action.
11. **Compact Live** — same hierarchy as Live at a narrower working size.
12. **Design language / performance view** — large musical actions, small engineering readouts, clear error cards; "Playing, not engineering."

## Supported layout checks

- Primary target: 1920x1080.
- Laptop target: 1366x768.
- PMX minimum application bounds remain 1100x680; below that the app should refuse further shrinking rather than collapsing controls.
- Overlay dialogs cap around 560x470 while preserving at least 40 px surrounding margin at supported sizes.
- Preset cards stay above the tested minimum width at both target resolutions.

The screenshot contains a light Live concept as well as the final dark Live direction. The shipped alpha follows the dark direction used by the majority of the reference screens and the final compact Live/design-language panels.
