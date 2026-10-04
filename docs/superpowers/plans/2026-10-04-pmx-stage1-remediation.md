# PMX Stage 1 remediation implementation plan

> Execute with superpowers:executing-plans, task by task, with regression tests before behavioral changes and a whole-branch review.

**Goal:** Make the existing native foundation safe, usable, visually faithful, and downloadable for Pocket Master acceptance testing.
**Architecture:** Retain components; separate UI-owned musical settings from callback-owned DSP/transport. Background asset preparation hands off outside the callback; small commands/parameters cross bounded or atomic interfaces.
**Tech Stack:** C++20, JUCE 9.0.3, pinned NAM core, CMake, Windows x64, Inno Setup.
**Spec:** `docs/superpowers/specs/2026-10-04-pmx-stage1-remediation.md`

## Global constraints

- Pocket Master is audio I/O only; no MIDI/firmware/device edits.
- Start/reconnect muted, no automatic laptop fallback.
- No callback allocations/locks/files/network/UI/model loading.
- Preserve current assets on error and all user data on upgrade.
- Reference layout at 1920x1080 and 1366x768; minimum 1100x680.
- Report source/tests/Windows build/installer/hardware states separately.

## Review focus

- UI edits during active processing: no data races or unbounded operations.
- Partial/undone overdubs and repeated clears: no stale audio or lost layers.
- Device disappearance while loading/recording: muted and recoverable.
- Corrupt/oversized assets and missing preset references: prior sound survives with clear error.
- Small window, empty libraries, modal/shortcut focus: legible, honest and safe.

### Task 1: local baseline and project hygiene
Files: `.gitignore`, `.github/workflows/windows-build.yml`, remove `.github/workflows/bootstrap-step1.yml`, `docs/ui/pmx-reference.png`, evidence documentation.
Interfaces: CMake test-only build; Windows GUI CI; ordinary source Git tree.
- [ ] Preserve supplied reference and record screen/style/layout mapping.
- [ ] Configure portable local test build and run unchanged 22-test baseline.
- [ ] Enable `codex/**` CI, remove obsolete transfer workflow; commit coherent project baseline.

### Task 2: callback ownership and transport safety
Files: `src/audio/ProcessingEngine.*`, `src/looper/LooperEngine.*`, `src/recording/QuickRecorder.*`, `src/dsp/effects/*`, `tests/audio/*`, `tests/looper/*`, `tests/recording/*`.
Interfaces: engine `requestLoopCommand`, published `loopStatus`, stopped snapshot; musical setters safe concurrently with processing.
- [ ] Add regressions for stale overdub after clear, partial overdub undo/redo, recorder concurrent stop, parameter changes and mute/invalid blocks; run RED.
- [ ] Implement bounded transport ownership, lazy layer commits, atomic parameter targets and producer shutdown; run GREEN and full suite.
- [ ] Wire runtime commands/status to the new interface; commit.

### Task 3: reference UI and real musical state
Files: `src/ui/*`, `src/presets/*`, `src/app/PmxRuntime.*`, `tests/ui/*`, `tests/presets/*`.
Interfaces: shared musical state for effect editor/preset round-trip; engine metrics/transport/tempo update real visual states.
- [ ] Add tests for full preset round-trip, missing assets, real layout bounds and modal/shortcut behavior; verify RED.
- [ ] Reproduce reference cards/navigation/modals; implement all effect editors, Mute, tempo/metronome, Quick Recorder, real waveform/level and searchable presets; preserve safe setup.
- [ ] Add JUCE render harness at target sizes, inspect emitted images, fix bounds; verify tests/build; commit.

### Task 4: safe assets, device lifecycle and usable DSP
Files: `src/app/PmxRuntime.*`, `src/audio/JuceAudioHost.*`, `src/nam/*`, `src/ir/*`, `src/dsp/*`, related tests.
Interfaces: worker-owned prepared asset results; explicit device state; prepared processing remains active through loads.
- [ ] Add malformed/oversized IR and NAM failure retention, preparation/reconnect preservation, silence/noise tuner, envelope and gain tests; verify RED.
- [ ] Move NAM/IR preparation to worker, install only when callback detached without device restart, preserve transport; improve validation and DSP; fix explicit ASIO/device loss policy.
- [ ] Run full suite and Windows GUI build; commit.

### Task 5: packaging, licensing, CI and acceptance evidence
Files: `README.md`, `installer/PMX.iss`, `.github/workflows/windows-build.yml`, `docs/testing/*`, third-party notices/license, updater tests.
Interfaces: downloadable artifact with binary/installer/checksums/notices; user data outside installation.
- [ ] Verify pinned licenses and installer data preservation; correct overclaims and add exact acceptance checklist.
- [ ] Run local suite, Windows x64 CI tests/GUI/render/installer; diagnose failures and verify fixes.
- [ ] Perform fresh whole-branch review; fix important findings with RED/GREEN tests.
- [ ] Commit/push milestone branch; provide verified download and evidence. Hardware acceptance remains owner-dependent.
