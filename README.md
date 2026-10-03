# PMX

A planned Windows 11 live guitar workstation. The Pocket Master is used only as a USB/ASIO audio interface; PMX handles PC-side effects and looping.

**Current status: design review. No runnable PMX installer has been released from this repository.**

## Start here

Read the [PMX written design](docs/superpowers/specs/2026-10-04-pmx-design.md).

The first usable increment is planned to include guitar effects, NAM/IR imports, a 120-second stereo looper, saved presets, local loop export, and an in-app Update button. Bass/synth/drums follow as tested instrument engines, followed by piano and violin. PMX Studio is a separate later project.

## Fixed boundaries

- Windows 11 x64, Pocket Master audio I/O only.
- No Pocket Master MIDI, preset editing, firmware changes, or footswitch integration.
- Local audio processing and user data; no account or cloud audio upload.
- Published-release updates with user-controlled installation; no Git commands for normal use.
- Protect user presets, imported assets and saved loops during upgrades.

The earlier v0.1 source archive is a prototype reference. Its reported core tests are not evidence of a working Windows executable. Implementation, test results and release downloads will be documented when they actually exist.
