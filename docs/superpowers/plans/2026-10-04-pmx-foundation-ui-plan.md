# PMX Foundation + Screenshot-Matched UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first Windows 11 PMX alpha that matches the supplied PMX reference image, uses Pocket Master as the USB/ASIO audio interface, supports the core guitar/live workflow, and is ready for hardware testing.

**Architecture:** Native C++20/JUCE desktop app with a real-time audio engine separated from UI/state/persistence. The supplied PMX screenshot collage is the visual source of truth for layout, spacing, color hierarchy, screen structure, cards, dialogs, and interaction density. The first alpha implements the Foundation and immediate Performance Tools needed for the first hardware test; later Bass/Synth/Drums/Piano/Violin engines attach to the same clean-analysis/audio graph.

**Tech Stack:** C++20, JUCE 9.0.3 pinned to `be29c81492b6151c8ea8d14c840e1311963b3a83`, CMake, Windows 11 x64, ASIO, Catch2 or JUCE UnitTest for deterministic core tests, GitHub Actions, Inno Setup, NeuralAmpModelerCore behind an adapter boundary.

**Spec:** `docs/superpowers/specs/2026-10-04-pmx-design.md`

## Global Constraints

- Windows 11 x64 is the acceptance platform.
- Pocket Master is audio I/O only: no MIDI, firmware, preset editing, or footswitch control.
- No fallback to laptop mic/speakers when Pocket Master disappears.
- No blocking locks, heap allocation, disk I/O, network I/O, or UI calls from the audio callback.
- Preserve presets, imported NAM/IR references, loops, recordings, and settings across updates.
- UI should match the supplied PMX design image rather than being independently redesigned.
- Default UI remains simple: Live, Looper, Presets, Settings, Update.
- First alpha must not claim complete Bass/Synth/Drums/Piano/Violin support.
- Real units are shown where meaningful: Hz, ms, dB, %, samples.
- Monitoring starts muted/safe until the user deliberately enables it.

## Review Focus

- Pocket Master disconnect/reconnect during active audio: PMX must mute/stop safely and never switch devices silently.
- Unexpected/unsupported ASIO channel/rate/buffer combinations: show a readable error and keep a safe prior configuration.
- Invalid or expensive NAM/IR files: reject off-thread and leave the last working rig untouched.
- Audio callback under pressure: no allocation/disk/network/UI work and no unsafe NaN/Inf output.
- User data during upgrade/crash: preserve settings/presets/assets/loops and recover the last stable state where possible.

---

### Task 1: Project scaffold, dependency pinning, CI, and app identity

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/Dependencies.cmake`
- Create: `src/app/Main.cpp`
- Create: `src/app/AppVersion.h.in`
- Create: `tests/CMakeLists.txt`
- Create: `.github/workflows/windows-build.yml`

**Interfaces:**
- Produces: `pmx::AppVersion::current()`; Windows `PMX.exe`; shared test target.

- [ ] **Step 1: Write failing smoke test**
  - Assert app version is non-empty and semantic-version parseable.
- [ ] **Step 2: Run configure/test and verify failure**
  - Run: `cmake -S . -B build -DPMX_BUILD_TESTS=ON && cmake --build build --config Debug && ctest --test-dir build -C Debug --output-on-failure`
  - Expected: FAIL before scaffold exists.
- [ ] **Step 3: Implement minimal CMake/JUCE scaffold**
  - Pin JUCE 9.0.3 commit exactly.
  - Define Windows ASIO compile support.
  - Create standalone GUI target and test target.
- [ ] **Step 4: Add Windows GitHub Actions build**
  - Configure, build Debug/Release, run tests, upload unsigned alpha artifact.
- [ ] **Step 5: Verify locally/CI-config syntax**
  - Expected: configure/tests pass where JUCE toolchain is available.
- [ ] **Step 6: Commit**
  - `feat: scaffold PMX Windows application`

### Task 2: Screenshot-derived design system and navigation shell

**Files:**
- Create: `src/ui/PmxTheme.h`
- Create: `src/ui/PmxTheme.cpp`
- Create: `src/ui/components/PmxButton.h/.cpp`
- Create: `src/ui/components/PmxCard.h/.cpp`
- Create: `src/ui/components/PmxMeter.h/.cpp`
- Create: `src/ui/components/PmxKnob.h/.cpp`
- Create: `src/ui/MainWindow.h/.cpp`
- Create: `src/ui/AppShell.h/.cpp`
- Test: `tests/ui/ThemeTests.cpp`

**Interfaces:**
- Produces: reusable dark PMX theme tokens and top navigation `Live | Looper | Presets | Settings` plus Update action.

- [ ] **Step 1: Add theme/token tests**
  - Assert accent, destructive, connected, panel, border, and text tokens are defined and stable.
- [ ] **Step 2: Run tests and verify failure**
- [ ] **Step 3: Implement theme from reference image**
  - Near-black window background.
  - Charcoal cards/panels.
  - Electric-blue primary accent.
  - Green connected/healthy state.
  - Red reserved for record/destructive/fault.
  - Compact top bar, rounded cards, restrained borders/shadows, high information hierarchy.
- [ ] **Step 4: Implement app shell**
  - Match screenshot structure: PMX wordmark left, tabs centered/left, compact version/connected/update area right.
  - Resizable layout usable at 1920x1080 and approximately 1366x768.
- [ ] **Step 5: Verify theme tests**
- [ ] **Step 6: Commit**
  - `feat: add screenshot-matched PMX design system`

### Task 3: First-run wizard matching the three reference screens

**Files:**
- Create: `src/ui/screens/FirstRunWizard.h/.cpp`
- Create: `src/state/SetupState.h`
- Test: `tests/state/SetupStateTests.cpp`

**Interfaces:**
- Consumes: future device discovery result.
- Produces: setup steps `Welcome -> Pocket Master Found -> You're Ready` and persisted first-run completion flag.

- [ ] **Step 1: Add state-transition tests**
  - Welcome cannot skip directly to Ready without a valid detected device/test state.
  - Back/continue retain safe monitoring state.
- [ ] **Step 2: Verify failure**
- [ ] **Step 3: Implement wizard UI to match screenshots**
  - Centered modal card.
  - Step indicator at top.
  - Welcome instruction and Continue.
  - Device-found screen with Input/Output/ASIO status cards and Test Audio.
  - Ready screen with selected starter preset and Start Playing.
- [ ] **Step 4: Verify tests**
- [ ] **Step 5: Commit**
  - `feat: add PMX first-run setup wizard`

### Task 4: Audio device layer and safe Pocket Master routing

**Files:**
- Create: `src/audio/AudioDeviceController.h/.cpp`
- Create: `src/audio/AudioStatus.h`
- Create: `src/audio/SafeAudioCallback.h/.cpp`
- Test: `tests/audio/AudioDeviceControllerTests.cpp`
- Test: `tests/audio/SafeAudioCallbackTests.cpp`

**Interfaces:**
- Produces:
  - `AudioDeviceController::availableDevices()`
  - `AudioDeviceController::open(const AudioDeviceSelection&)`
  - `AudioDeviceController::close()`
  - `AudioStatus AudioDeviceController::status() const`
  - callback interface for downstream processing.

- [ ] **Step 1: Write device-state tests**
  - No silent fallback.
  - Unsupported config returns readable error.
  - Disconnect transitions to muted/disconnected.
- [ ] **Step 2: Write sample-safety tests**
  - NaN/Inf input cannot reach output.
- [ ] **Step 3: Verify failures**
- [ ] **Step 4: Implement device enumeration/open/close/reconnect state**
  - Prefer Pocket Master only as acceptance target, but display enumerated names exactly.
  - Start with output muted.
- [ ] **Step 5: Implement callback safety wrapper**
- [ ] **Step 6: Verify tests**
- [ ] **Step 7: Commit**
  - `feat: add safe ASIO audio device layer`

### Task 5: Core live audio graph, bypass, master protection, and meters

**Files:**
- Create: `src/audio/ProcessingEngine.h/.cpp`
- Create: `src/audio/SignalMetrics.h/.cpp`
- Create: `src/dsp/OutputProtector.h/.cpp`
- Test: `tests/audio/ProcessingEngineTests.cpp`
- Test: `tests/dsp/OutputProtectorTests.cpp`

**Interfaces:**
- Produces:
  - `ProcessingEngine::prepare(double sampleRate, int maxBlockSize, int channels)`
  - `ProcessingEngine::process(AudioBufferView)`
  - `ProcessingEngine::setFxBypass(bool)`
  - `ProcessingEngine::setMuted(bool)`
  - lock-free meter snapshot.

- [ ] **Step 1: Add bypass/mute/protection tests**
  - FX bypass preserves clean signal within tolerance.
  - Mute returns silence.
  - Protection contains non-finite/overload samples.
- [ ] **Step 2: Verify failure**
- [ ] **Step 3: Implement processing graph skeleton**
- [ ] **Step 4: Implement peak/RMS meter snapshots without UI calls from callback**
- [ ] **Step 5: Verify tests**
- [ ] **Step 6: Commit**
  - `feat: add PMX live processing engine`

### Task 6: Guitar rack and effect editor matching the Live/Delay reference

**Files:**
- Create: `src/dsp/GuitarRack.h/.cpp`
- Create: `src/dsp/effects/Gate.h/.cpp`
- Create: `src/dsp/effects/Compressor.h/.cpp`
- Create: `src/dsp/effects/Drive.h/.cpp`
- Create: `src/dsp/effects/Eq.h/.cpp`
- Create: `src/dsp/effects/Chorus.h/.cpp`
- Create: `src/dsp/effects/Delay.h/.cpp`
- Create: `src/dsp/effects/Reverb.h/.cpp`
- Create: `src/ui/screens/LiveScreen.h/.cpp`
- Create: `src/ui/dialogs/EffectEditor.h/.cpp`
- Test: `tests/dsp/GuitarRackTests.cpp`

**Interfaces:**
- Produces fixed rack order:
  `Gate -> Comp -> Drive -> NAM -> IR -> EQ -> Mod -> Delay -> Reverb`
  with module bypass and parameter-state API.

- [ ] **Step 1: Add rack-order/state-roundtrip tests**
- [ ] **Step 2: Add delay timing/feedback stability tests**
- [ ] **Step 3: Verify failures**
- [ ] **Step 4: Implement minimal safe DSP modules and smoothed parameters**
- [ ] **Step 5: Build Live screen from screenshot**
  - Large preset title such as DREAM CLEAN.
  - Compact module cards across the middle.
  - NAM and IR cards beneath.
  - Connected/status strip.
  - Save Preset / Bypass controls.
- [ ] **Step 6: Build Delay editor modal from screenshot**
  - Dark centered dialog, three large knobs, tap-tempo action, Advanced disclosure, Done.
- [ ] **Step 7: Verify tests**
- [ ] **Step 8: Commit**
  - `feat: add guitar rack and live UI`

### Task 7: Tuner and tempo/metronome services

**Files:**
- Create: `src/analysis/TunerEngine.h/.cpp`
- Create: `src/audio/TempoService.h/.cpp`
- Create: `src/dsp/Metronome.h/.cpp`
- Create: `src/ui/components/TunerView.h/.cpp`
- Test: `tests/analysis/TunerEngineTests.cpp`
- Test: `tests/audio/TempoServiceTests.cpp`

**Interfaces:**
- Produces:
  - `TunerResult { noteName, frequencyHz, cents, confidence }`
  - shared BPM/tap-tempo clock.
- [ ] **Step 1: Add deterministic tone tests for 82.41, 110, 440 Hz**
- [ ] **Step 2: Add low-confidence/no-signal tests**
- [ ] **Step 3: Add tap-tempo convergence tests**
- [ ] **Step 4: Verify failures**
- [ ] **Step 5: Implement analysis and tempo services off the clean-input tap**
- [ ] **Step 6: Integrate tuner/metronome into Live UI**
- [ ] **Step 7: Verify tests**
- [ ] **Step 8: Commit**
  - `feat: add tuner and shared tempo service`

### Task 8: NAM/IR adapters and screenshot-matched amp/cab browser

**Files:**
- Create: `src/assets/AssetLibrary.h/.cpp`
- Create: `src/nam/NamProcessor.h/.cpp`
- Create: `src/ir/IrProcessor.h/.cpp`
- Create: `src/ui/dialogs/NamIrBrowser.h/.cpp`
- Test: `tests/assets/AssetLibraryTests.cpp`
- Test: `tests/ir/IrProcessorTests.cpp`

**Interfaces:**
- Produces prepared, immutable model/IR objects swappable at a safe audio boundary.
- [ ] **Step 1: Add invalid/missing-file tests**
- [ ] **Step 2: Add IR decode/sample-rate tests**
- [ ] **Step 3: Verify failures**
- [ ] **Step 4: Implement local library metadata and off-thread prepare pipeline**
- [ ] **Step 5: Integrate NeuralAmpModelerCore behind `NamProcessor` adapter**
- [ ] **Step 6: Implement browser matching screenshot**
  - Two-column Amp Model / Cabinet IR selection.
  - Search, selected state, import actions, Load Selected Pair.
- [ ] **Step 7: Verify tests**
- [ ] **Step 8: Commit**
  - `feat: add NAM and IR library workflow`

### Task 9: Preset persistence and screenshot-matched preset browser

**Files:**
- Create: `src/presets/Preset.h`
- Create: `src/presets/PresetStore.h/.cpp`
- Create: `src/ui/screens/PresetsScreen.h/.cpp`
- Test: `tests/presets/PresetStoreTests.cpp`

**Interfaces:**
- Produces versioned preset JSON with stable ID, display name, rack parameters, mode, and asset hashes/references.
- [ ] **Step 1: Add save/load roundtrip and missing-asset tests**
- [ ] **Step 2: Add atomic-write/backup test**
- [ ] **Step 3: Verify failure**
- [ ] **Step 4: Implement store and current-rig dirty-state tracking**
- [ ] **Step 5: Implement preset grid from screenshot**
  - Categories left.
  - Search/filter top.
  - Large tone cards.
  - Favorite indicator.
  - Save Preset action.
- [ ] **Step 6: Verify tests**
- [ ] **Step 7: Commit**
  - `feat: add PMX preset library`

### Task 10: Looper and screenshot-matched Looper screen

**Files:**
- Create: `src/looper/LooperEngine.h/.cpp`
- Create: `src/looper/LoopExportService.h/.cpp`
- Create: `src/ui/screens/LooperScreen.h/.cpp`
- Test: `tests/looper/LooperEngineTests.cpp`

**Interfaces:**
- Produces states Empty/Recording/Playing/Overdubbing/Stopped and one-level overdub Undo/Redo.
- [ ] **Step 1: Add state-transition tests**
- [ ] **Step 2: Add max-length, overdub-isolation, undo/redo tests**
- [ ] **Step 3: Verify failures**
- [ ] **Step 4: Implement preallocated 120-second stereo loop engine**
- [ ] **Step 5: Implement worker-based Save/Export WAV**
- [ ] **Step 6: Implement UI from screenshot**
  - Large waveform/progress panel.
  - 00:13 / 00:42 style timing.
  - Large Record/Play/Overdub/Stop tiles.
  - Smaller Undo/Redo/Clear actions.
  - Loop level slider and Save Loop/Export WAV.
- [ ] **Step 7: Verify tests**
- [ ] **Step 8: Commit**
  - `feat: add PMX live looper`

### Task 11: Quick Recorder, Settings screen, and audio diagnostics

**Files:**
- Create: `src/recording/QuickRecorder.h/.cpp`
- Create: `src/ui/screens/SettingsScreen.h/.cpp`
- Create: `src/audio/AudioDiagnosticService.h/.cpp`
- Test: `tests/recording/QuickRecorderTests.cpp`
- Test: `tests/audio/AudioDiagnosticServiceTests.cpp`

**Interfaces:**
- Produces continuous master-output recording with worker-thread disk writes and readable device/latency diagnostics.
- [ ] **Step 1: Add long-recording and interrupted-write tests**
- [ ] **Step 2: Add device diagnostic-state tests**
- [ ] **Step 3: Verify failures**
- [ ] **Step 4: Implement recorder ring-buffer/file worker**
- [ ] **Step 5: Implement Settings screen from screenshot**
  - Connection card.
  - Input/output/channel/buffer selections.
  - Driver-reported latency card.
  - Input/output gain controls.
  - readable fault card and Try Again/Show Details.
- [ ] **Step 6: Verify tests**
- [ ] **Step 7: Commit**
  - `feat: add recorder and audio diagnostics`

### Task 12: Update UI, release checker, installer handoff, and packaging

**Files:**
- Create: `src/update/ReleaseChecker.h/.cpp`
- Create: `src/update/UpdateState.h`
- Create: `src/ui/dialogs/UpdateDialog.h/.cpp`
- Create: `installer/PMX.iss`
- Test: `tests/update/ReleaseCheckerTests.cpp`

**Interfaces:**
- Produces nonblocking manual/startup update state and verified installer handoff.
- [ ] **Step 1: Add version/offline/digest-mismatch tests**
- [ ] **Step 2: Verify failures**
- [ ] **Step 3: Implement GitHub Releases metadata check outside audio thread**
- [ ] **Step 4: Implement download verification and safe install handoff**
- [ ] **Step 5: Implement update dialog from screenshot**
  - "A little better. Still your sound."
  - What's New section.
  - installed/available versions.
  - Not Now / Update.
- [ ] **Step 6: Add per-user Inno Setup installer preserving user data**
- [ ] **Step 7: Verify tests**
- [ ] **Step 8: Commit**
  - `feat: add PMX update and installer flow`

### Task 13: Performance screen polish, keyboard shortcuts, and screenshot fidelity pass

**Files:**
- Create: `src/input/ShortcutManager.h/.cpp`
- Create: `src/ui/screens/PerformanceMode.h/.cpp`
- Modify: all UI screen files above
- Test: `tests/input/ShortcutManagerTests.cpp`

**Interfaces:**
- Produces Bypass, Mute, Tuner, loop transport, Tap Tempo, Quick Record shortcuts.
- [ ] **Step 1: Add shortcut focus-safety tests**
- [ ] **Step 2: Verify failure**
- [ ] **Step 3: Implement shortcuts and performance mode**
- [ ] **Step 4: Run screenshot-fidelity pass against supplied image**
  - Check hierarchy, card geometry, modal size, top navigation, spacing, alignment, accent usage, and text density.
  - Do not invent a new visual direction.
- [ ] **Step 5: Verify UI at 1920x1080 and 1366x768**
- [ ] **Step 6: Commit**
  - `feat: finish PMX foundation UI fidelity pass`

### Task 14: Release-gate verification for first hardware alpha

**Files:**
- Create: `docs/testing/first-pocket-master-test.md`
- Modify: `README.md`
- Modify: `.github/workflows/windows-build.yml`

**Interfaces:**
- Produces first installable alpha artifact and explicit hardware-test checklist.
- [ ] **Step 1: Run complete Release build and test suite**
  - `cmake --build build --config Release && ctest --test-dir build -C Release --output-on-failure`
- [ ] **Step 2: Build installer**
- [ ] **Step 3: Verify clean install/upgrade preserves user-data directory**
- [ ] **Step 4: Publish no production claim yet**
  - Label artifact alpha/test build.
- [ ] **Step 5: Document the exact user hardware test**
  - Pocket Master detect/open.
  - Clean USB return.
  - No doubled monitoring/feedback.
  - FX bypass/mute.
  - NAM/IR.
  - Tuner.
  - Looper.
  - Presets.
  - Metronome/tap.
  - Quick recorder.
  - 128-sample stability and optional 64/256 tests.
  - Disconnect/reconnect.
- [ ] **Step 6: Commit**
  - `test: define first Pocket Master hardware acceptance run`

## Follow-on Plans After First Hardware Alpha

These are deliberately separate plans after the foundation is accepted:

1. `pmx-instrument-engine-plan.md` — Bass, Synth, Drums/Percussion, Piano, Violin.
2. `pmx-library-recovery-plan.md` — autosave, crash recovery, backup/restore, A/B compare, expanded asset library.
3. `pmx-update-recovery-plan.md` — tested previous-version rollback/recovery hardening.
4. Future separate PMX Studio design/plan — simplified DAW workflow.

