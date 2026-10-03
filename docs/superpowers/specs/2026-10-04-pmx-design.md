# PMX: Windows Live Guitar Workstation

**Design date:** 2026-10-04  
**Repository:** KaranBarua01/PMX  
**Status:** Expanded written design for owner review. This document is not an implementation or a released installer.

## 1. Purpose and agreed boundaries

PMX is a simple Windows 11 application for playing guitar through effects, transforming guitar performance into other instrument sounds, recording quick ideas, and building loops. The user should be able to install it, select the audio device, choose a sound, and play without learning a traditional DAW or using Git commands.

The Sonicake Pocket Master is strictly the USB/ASIO audio interface. PMX must not send MIDI, edit the device, change its presets, update its firmware, or depend on its footswitches. All PMX processing, instrument transformation, recording, looping, preset management, and update controls live on the PC. A one-time manual clean/bypass configuration on the Pocket Master may be necessary for its audio routing; this is not automated by PMX.

The accepted live-app modes are Guitar, Bass, Synth, Drums/Percussion, Piano, and Violin. A future Studio workspace can reuse the engine, but multitrack recording and arrangement are a separate later project.

**Success:** a reliable, understandable live-music application where the normal workflow is:

```text
Tune -> choose mode/sound -> play -> add effects -> loop or record -> save
```

A rendered interface, successful compilation, or prototype sound alone does not establish success.

## 2. Product architecture and delivery order

Everything below is part of PMX, but it is delivered in testable modules so a failure can be isolated instead of hidden inside a large unverified build.

| Milestone | Included | Release gate |
|---|---|---|
| 1. Core Live Engine | Windows audio setup; Pocket Master ASIO; clean bypass; tuner; guitar effects; NAM/IR; master/output protection; audio/latency diagnostics; presets | Windows build, automated DSP/state tests, and a Pocket Master routing/listening test |
| 2. Performance Tools | 120-second stereo looper; metronome; tap tempo; quick recorder; keyboard shortcuts; performance mode; undo/redo; favorites/recent sounds | Stable playback/recording, no blocking on audio thread, file-save tests, practical playing test |
| 3. Instrument Transformation | Guitar-controlled bass, synth, drums/percussion, piano and violin | Tracking, note/onset accuracy, false-trigger testing, measured added processing delay, practical playing test |
| 4. Library and Reliability | NAM/IR library management; Save As/Duplicate/A-B compare; autosave; crash recovery; backup/restore; recordings/loops library | Recovery/migration tests, missing-asset tests, data-preservation test |
| 5. Update and Recovery | Startup update check; Update button; GitHub Releases; user-controlled install; previous-version recovery | Installer/update/digest/recovery tests and preservation of all user-created files |
| Later separate project | PMX Studio simplified recording/arrangement workspace | Separate written design and implementation plan |

Milestones define build order, not removal of features. Instrument modes remain committed product goals. Unfinished modes must be hidden or explicitly marked experimental; they must not masquerade as finished sounds.

The first candidate will use an alpha version such as `0.2.0-alpha.1`, not a misleading production `1.0`. Version 1.0 requires the accepted live-app scope to pass its release gates.

## 3. Technical decisions

Use a native C++20/JUCE standalone application, CMake builds, and Windows x64 distribution. Retain JUCE 9.0.3 as the initial candidate dependency, pinned to commit `be29c81492b6151c8ea8d14c840e1311963b3a83`. The official 9.0.3 release exists; pinning this version does not imply that the old PMX prototype has passed a Windows build. [1]

Use the Pocket Master manufacturer's ASIO driver already installed on the PC. Do not bundle or silently install third-party drivers. Enumerate the driver and channel names; do not hard-code an unverified device string. Other enumerated devices can appear in settings, but only Windows 11 with the Pocket Master is the acceptance target.

Use NeuralAmpModelerCore for NAM inference rather than reverse-engineering the Pocket Master's firmware. Its exact commit and tested model formats must be recorded before integration is released. [2]

Use an installer rather than requiring a source build. The proposed packaging tool is Inno Setup in per-user mode. Packaging-tool and dependency notices must accompany distributions. [3]

JUCE offers AGPLv3 and commercial licensing paths. The proposed public-source distribution path is AGPLv3-compatible, with corresponding source and third-party notices for every binary release. Verify the exact JUCE, ASIO integration, NAM, sample-asset, and packaging dependency terms before publishing binaries. Incompatible terms block a release. Do not purchase licences without approval. [4]

## 4. Audio routing and component boundaries

### Live signal path

```text
Guitar
  -> Pocket Master audio input
  -> USB / ASIO input channel selected in PMX
  -> clean analysis tap
       -> tuner
       -> pitch/onset tracker
       -> instrument engine when selected
  -> live processing path
       -> input trim -> gate -> compressor -> drive
       -> NAM (optional) -> cabinet IR (optional)
       -> EQ -> modulation -> delay -> reverb
  -> looper / quick-recorder taps
  -> master level -> output protection
  -> USB / ASIO stereo output
  -> Pocket Master headphones or speaker connection
```

Instrument analysis uses a clean pre-distortion tap. Instrument-generated audio is mixed deliberately with or instead of the original guitar; dry leakage must never be accidental.

Stereo spatial effects follow mono guitar-processing stages. A loop captures the processed live output before master level and output protection. Playback rejoins after the live rack so changing a live preset does not reprocess an already-recorded loop.

### Responsibilities

| Component | Owns | Must not own |
|---|---|---|
| Audio device layer | ASIO connection, supported rates/buffers, channels, reconnect state | Presets, downloads, instrument selection |
| Processing engine | Block processing, parameters, module bypass, graph state | Windows, file dialogs, network calls |
| Analysis engine | Tuner, pitch tracking, onset tracking, confidence/state | Device control, UI drawing |
| Guitar rack | Gate, compressor, drive, NAM, IR, EQ, modulation, delay, reverb | Device control |
| Instrument engines | Bass, synth, drum, piano, violin generation | Hardware control, update installation |
| Looper | Loop transport, audio buffers, overdub, undo/redo state | Direct file writes inside callback |
| Recorder | Continuous quick-take capture and stable handoff to file worker | UI ownership, network calls |
| Tempo service | BPM, tap tempo, metronome timing and sync events | Audio device configuration |
| Preset/asset store | Versioned settings, file validation, favorites, backups | Audio-thread mutation of large objects |
| User interface | Controls, readable status, user commands | Ownership of live audio buffers |
| Update service | Release lookup, verified download, installer handoff | Real-time processing, firmware operations |

The processing engine exposes preparation, block-processing, reset, command, and parameter boundaries without depending on the UI. A future Studio host can reuse these boundaries, but that is not a promise of zero-change DAW conversion.

## 5. Audio safety and performance contract

Start with monitoring muted and a conservative master level. Let the user explicitly enable monitoring after selecting input and output. Do not fall back silently to the laptop microphone or speakers if the Pocket Master disappears.

Request 44.1 kHz and 128 samples when the selected ASIO driver offers them. Offer only supported alternatives; 64 and 256 samples are test candidates, not guaranteed capabilities. Show the actual selected configuration.

Read driver-reported input and output latency separately. Report their sum as **driver-reported I/O latency**, with known processing delay shown separately. Never label buffer duration, CPU load, or a guessed number as measured end-to-end latency. [5]

No locks that can block, heap allocation, buffer resizing, large clearing/copying, disk access, network requests, or UI calls are permitted in the audio callback. Preallocate working buffers and use bounded commands. Prepare NAM/IR/sample replacements off the audio thread and swap only at a safe boundary.

Provide separate **FX Bypass** and **Mute** controls. FX Bypass skips the live rack but retains master protection and any playing loop. Mute silences the entire output.

Output protection must reject non-finite samples and contain digital overload. It is not a guarantee against excessive physical headphone volume.

Losing the audio device mutes output, stops active transport safely, preserves recoverable state, and presents an explicit reconnect action. PMX must not silently substitute another input/output device.

The first hardware test must establish clean USB send, audible USB return, and monitoring without doubled dry guitar or feedback. If the Pocket Master's direct-monitor path cannot be disabled or managed suitably, document that limitation before claiming interface-only operation works.

## 6. Core Live Engine

### 6.1 Tuner

Provide a large chromatic tuner accessible from Live and via keyboard shortcut. Show note name, cents sharp/flat, confidence/stability, and input level. Tuner analysis comes from the clean input tap.

Tuner mode may optionally mute the live output, but muting must be explicit and reversible. No tuning result is shown when signal confidence is too low.

### 6.2 Guitar rack

Use a fixed understandable rack order:

```text
INPUT -> GATE -> COMP -> DRIVE -> NAM -> IR -> EQ -> MOD -> DELAY -> REVERB -> MASTER
```

Each module has an on/off state and a compact primary control set, with detailed controls under Advanced.

| Module | Controls shown with real units |
|---|---|
| Input/gate | Input dB, threshold dB, attack/release ms |
| Compressor | Threshold dB, ratio, attack/release ms, makeup dB |
| Drive | Drive amount, tone, output dB |
| NAM | File/name, input trim dB, output trim dB, compatibility status |
| IR | File/name, low-cut Hz, high-cut Hz, output dB |
| EQ | Gain dB and frequency Hz |
| Modulation | Actual algorithm parameters, including rate in Hz when applicable |
| Delay | Time ms or tempo-sync division, feedback %, mix % |
| Reverb | Actual algorithm parameters and mix % |
| Master | Output dB, output meter, clip state, Mute |

Never use arbitrary 0-100 displays where the DSP has meaningful physical units unless the algorithm genuinely defines a normalized control.

### 6.3 NAM and IR

NAM and IR imports use local `.nam` and `.wav` files. PMX does not extract models from the Pocket Master and does not assume files previously imported into the pedal are present on the PC.

Validate model architecture/configuration, finite data, file size, sample-rate requirements, and processing cost. Do not claim universal A1/A2 compatibility until tested.

Decode and prepare IRs outside the callback. Reject malformed or unsupported files without replacing the last working sound.

Presets store asset hashes/references plus module state. Missing assets produce a repair prompt rather than silent substitution.

### 6.4 Master and diagnostics

Live always exposes master level, output meter, Mute, and overload state. A compact status area shows actual sample rate, buffer size, driver-reported latency, and audio-engine load.

Settings provides an audio test that confirms selected Pocket Master input/output, reports supported buffer/rate options, and guides the user toward 64/128/256-sample choices with human-readable descriptions. PMX must not invent an "excellent" latency rating without defined measured criteria.

CPU overload handling should first warn, then offer safe actions such as increasing the buffer or disabling expensive modules. It must not randomly disable user-selected effects without explanation.

## 7. Performance tools

### 7.1 Looper

The looper is one stereo loop up to 120 seconds, with Record, Play, Overdub, Stop, Undo, Redo, Clear, loop level, Save Loop, and Export WAV.

States are Empty, Recording, Playing, Overdubbing, and Stopped. The first recording establishes loop length. Overdubbing records only the new live contribution, not loop playback fed back into itself.

Undo/Redo applies to the most recent completed overdub. Large loop copies/clears are not performed in the callback. Loop boundaries are smoothed to avoid avoidable clicks.

Export is 24-bit stereo PCM WAV at the session rate. Internal reopenable saves may use lossless float WAV plus metadata.

### 7.2 Metronome and tap tempo

Provide BPM, Start/Stop, volume, accent, and Tap Tempo. Default BPM range is practical for normal music use and validated in implementation tests.

Tempo is a shared service so delay and later beat/sequencer features can follow the same BPM. Effects may choose milliseconds or musical divisions. Tap tempo never blocks audio processing.

### 7.3 Quick Recorder

Quick Recorder is separate from the looper. It captures the master program output as a continuous take without overdub semantics.

Controls: Record, Stop, elapsed time, save location, and file name. It must support takes longer than the looper limit. Recording uses preallocated/ring-buffer handoff to a file worker so disk writes never block the callback.

An interrupted or crashed session should recover as much of the temporary recording as safely possible.

### 7.4 Keyboard shortcuts and performance mode

Provide configurable or documented shortcuts for at least Bypass, Mute, Tuner, Loop Record/Play, Stop, Tap Tempo, and Quick Record. Shortcuts are disabled when focus is in text entry or a destructive confirmation dialog.

Performance mode shows only the current sound/mode, tuner access, essential meters, master state, large loop controls, tempo, and a small preset switcher. It is optimized for playing, not editing.

## 8. Instrument Transformation

All instrument modes use the clean-input analysis path and expose confidence/sensitivity controls appropriate to the engine.

### 8.1 Common tracker contract

Start with monophonic note tracking for Bass, Synth, Piano, and Violin. Provide configurable gate/sensitivity, stable note transitions, retrigger control, pitch-bend smoothing where appropriate, and an **All Notes Off** safety action.

Measure instrument-analysis/generation delay separately from ASIO I/O delay. Do not promise universal low-latency numbers, especially on low guitar notes.

Each mode exposes Dry/Wet or Guitar/Instrument mix so the original guitar is included deliberately.

### 8.2 Bass

Bass mode derives a lower-pitched instrument voice from the played note, with octave-down behavior and a purpose-built bass tone path. It must not simply run the guitar through a cabinet and call it bass.

Initial controls: Sound, Tone, Attack, Sustain, Drive, and Guitar/Instrument mix.

### 8.3 Synth

Synth mode maps tracked guitar notes to a synthesizer voice. Initial controls: Sound, Tone, Attack, Sustain, Space, and Guitar/Instrument mix.

Advanced synthesis parameters remain behind Advanced. The default workflow is choose sound -> adjust character -> play.

### 8.4 Drums and percussion

The original PMX concept is preserved: guitar attacks can trigger actual drum/percussion sounds.

Initial playable sounds include Kick, Snare, Hi-Hat, Tom, Clap, and Crash.

Support two mapping methods:

1. **Note-range mapping:** different detected note ranges trigger different percussion sounds.
2. **User calibration mapping:** the user plays the desired string/note/gesture during setup and assigns that detected event to a drum sound.

Do not claim PMX can identify the physical guitar string with certainty from a mixed pickup signal when the same pitch can be produced in multiple positions. The UI may present friendly string-oriented mapping when calibration makes it practical, but the engine truth remains pitch/onset-based.

Drum mode requires onset detection, retrigger suppression, velocity estimation, sensitivity, minimum retrigger interval, and per-pad level. It triggers original or redistribution-permitted samples or synthesis; compression alone is not called a kick/snare conversion.

A small beat sequencer may be added after live triggering is stable. It remains a drum-machine workflow, not a MIDI piano roll.

### 8.5 Piano

Piano mode maps monophonic tracked notes to a sample-based or otherwise high-quality piano engine. Do not label a simple oscillator as realistic piano.

Initial controls: Sound, Tone, Attack, Sustain, Space, and Guitar/Instrument mix.

Chord transcription is not an initial requirement. Better polyphonic tracking can be a later enhancement after the monophonic engine is accepted.

### 8.6 Violin

Violin mode maps tracked notes to a string/violin engine with smooth pitch transition and expressive envelope behavior.

Initial controls: Sound, Tone, Attack, Sustain, Expression/Space, and Guitar/Instrument mix.

Automatic bow articulation recognition is not an initial requirement. Sound quality must be judged against the actual engine/assets rather than marketing labels.

## 9. Presets, library, and editing workflow

Presets store the complete PMX sound state for the applicable mode, including asset references and module bypass states. Loading a sound preset must not unexpectedly switch the audio device or global master-safety state.

Provide Save, Save As, Rename, Duplicate, Delete, Import, Export, Favorite, and Recent. Factory starter presets are read-only templates; user presets are editable.

Provide **A/B Compare** so the current edited state can be compared with the saved state without losing either. Provide app-level Undo/Redo for parameter editing where practical; destructive library operations still require explicit confirmation.

Organize presets into friendly categories such as Acoustic, Clean, Blues, Rock, Metal, Bass, Synth, Drums, Piano, Violin, and Favorites rather than presenting hundreds of undifferentiated rows.

### 9.1 NAM/IR library

Imported NAMs and IRs appear in a reusable local library. Provide Search, Favorite, Rename display label, Remove from library, Reveal file/details, compatibility status, and missing-file repair.

Deleting a library entry does not silently delete an original source file outside PMX-managed storage. Managed-copy deletion must be explicit.

### 9.2 Loop and recording library

Saved loops and quick recordings have a simple browser with name, date, duration, sample rate, file location, and play/reveal actions. PMX is not a media-library cloud service.

## 10. Autosave, crash recovery, backup, and local data

Install application files separately from user data. Keep settings, versioned preset JSON, managed imported assets, autosave/recovery state, and update staging under an appropriate per-user LocalAppData location. Use Windows known folders or user-selected paths for exported recordings.

Preset/settings records have schema versions and stable IDs. Use temporary files plus atomic replacement for metadata and keep a known-good backup. Refuse unsupported newer schemas rather than overwriting them.

Autosave the current editable rig and important UI/session state without blocking the audio thread. After an abnormal exit, PMX offers recovery of the last autosaved rig and any recoverable quick recording/unsaved loop state.

Provide **Backup PMX Library** and **Restore PMX Library**. A backup includes presets/settings metadata and optionally managed user assets/loops according to user choice. Restore shows what will be changed before overwriting current data.

Normal audio processing, looping, recording, presets, and local instrument assets work offline. Only update checks/downloads need network access. No account, telemetry, audio upload, automatic cloud sync, or embedded GitHub token is required.

## 11. Simple interface

Primary destinations are **Live**, **Looper**, **Presets**, **Settings**, and **Update**. Performance mode is entered from Live. The future Studio area is not shown until that project exists.

**Live:** selected mode/sound, tuner access, meters, rack/module switches, master, FX Bypass, Mute, tempo, compact loop/record controls.

**Looper:** large Record/Play/Overdub, Stop, Undo/Redo, Clear, loop level, elapsed/total time, progress, Save Loop, Export WAV.

**Presets:** category cards, favorites/recent, Save/Save As/Duplicate, A/B Compare, import/export.

**Settings:** device/ASIO control panel, input channel, output pair, rate/buffer, audio test, storage, diagnostics, shortcuts, backup/restore, update preferences.

**Update:** installed version, available version, release notes, Check, Download, Install/Restart, recovery status.

Errors use plain language first with optional technical details. Example:

```text
Pocket Master could not be opened.
Another audio application may be using it.

[TRY AGAIN]   [SHOW DETAILS]
```

The default interface hides engineering complexity. Advanced panels expose deeper controls without turning PMX into a DAW.

Support resizable windows, Windows display scaling, keyboard navigation, and practical operation at both 1920x1080 and approximately 1366x768.

## 12. Update design: no Git commands for normal use

Use GitHub Releases in this repository as the distribution source. [6][7]

Perform a nonblocking startup check at most once in 24 hours and provide a manual Check button. The setting can be disabled. Never install merely because PMX starts.

Stable builds ignore prereleases. Alpha builds use an explicitly labelled Preview channel. Compare semantic versions, not plain strings.

Show release notes, version, and download size before download/install. Select only the expected Windows x64 installer asset. Require HTTPS and verify the asset digest before execution. Integrity failure blocks installation.

The user presses Install and Restart. Block installation during recording, overdubbing, playback, or an unsaved recovery operation; offer safe stop/save first.

Updates preserve presets, imported assets, settings, saved loops, and recordings. Keep the previous verified installer plus a versioned settings backup. A failed download leaves the running version unchanged. A failed install offers previous-version recovery once that recovery path has passed its own tests.

No installer is offered merely because a commit exists. Only a published release with required checks and a valid release asset is eligible.

## 13. Testing and release evidence

The old `PMX_v0.1_source.zip` remains reference material, not proof of a verified Windows product. Any reusable code is re-reviewed and retested.

Evidence categories remain separate:

| Evidence | What it establishes |
|---|---|
| Unit/DSP tests | Particular behaviours on defined inputs |
| Windows CI build | Source can produce the Windows application |
| Installer/update test | Install, upgrade, preservation, and recovery paths work |
| Pocket Master hardware test | Actual routing, monitoring, latency, and listening quality on the user's setup |
| Musical playability test | Tuner/tracker/looper/instrument behavior feels usable while actually playing guitar |

Core tests include:

- Empty input, invalid samples, variable blocks, device loss, reconnect, and safe output.
- Channel selection, supported rate/buffer changes, no silent microphone fallback.
- Tuner accuracy/stability on defined tones.
- Guitar-effect parameter/state round trips and bypass behavior.
- Invalid/missing NAM/IR assets and safe replacement.
- Looper state transitions, zero/short/max recordings, overdub isolation, undo/redo, export failure.
- Metronome timing and tap-tempo convergence.
- Quick Recorder long capture, disk failure, interrupted write, and recovery.
- Preset Save As/Duplicate/A-B/Undo/Redo and library missing-file repair.
- Autosave and crash-recovery scenarios.
- Bass/synth/piano/violin note transitions, stuck-note prevention, and measured added delay.
- Drum onset false positives, retrigger suppression, mapping/calibration, and velocity behavior.
- Update offline/up-to-date/available/preview/digest-failure/interrupted-download scenarios.
- Installer upgrade over an existing user library and previous-version recovery.

Release tests must remain effective in Release configuration. Windows CI cannot replace the user's physical Pocket Master test.

## 14. First hardware acceptance test

The first installable alpha is intentionally narrower than the complete product. It must prove the audio foundation before instrument transformation is layered on top.

The user's first test checks:

1. PMX detects the Pocket Master ASIO device.
2. Correct input/output channels can be selected.
3. Clean monitored guitar reaches the Pocket Master output.
4. No unwanted doubled dry monitoring or feedback occurs.
5. FX Bypass and Mute behave correctly.
6. Guitar rack processing is audible and stable.
7. NAM and IR files can be loaded safely.
8. Tuner responds correctly.
9. Looper can record/play/overdub/stop/clear.
10. Presets save and reload.
11. Metronome/tap tempo and Quick Recorder work.
12. 128-sample playback is stable; 64 and 256 are tested only if the driver exposes them.
13. Device disconnect/reconnect does not crash the app.

Only after this test is accepted do Bass/Synth/Drums/Piano/Violin depend on the same live engine.

## 15. Explicit non-goals

No Pocket Master editor, firmware modifications, MIDI/Bluetooth integration, phone app, web-audio frontend, full multitrack timeline, piano roll, notation editor, VST host, plugin marketplace, cloud account, paid subscription, or automatic model downloads in the live-app build.

PMX Studio is a later separate design. A simple Quick Recorder is not a DAW.

## 16. Review checkpoint

This document records the complete accepted PMX live-app direction and the modular build order. The next artifact is a task-by-task implementation plan for Milestone 1 and its immediate Performance Tools dependencies, followed by implementation and the first Windows/Pocket Master alpha test.

Product code and release publication begin only after owner review of this written specification and the implementation plan.

## Primary references

These references support dependency/API facts. Product behaviours and release gates above are PMX requirements, not claims that those features already exist.

1. [JUCE 9.0.3 release](https://github.com/juce-framework/JUCE/releases/tag/9.0.3)
2. [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore)
3. [Inno Setup: PrivilegesRequired](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm)
4. [JUCE licensing](https://juce.com/get-juce/)
5. [JUCE AudioIODevice](https://docs.juce.com/master/classjuce_1_1AudioIODevice.html)
6. [GitHub Releases REST API](https://docs.github.com/en/rest/releases/releases)
7. [GitHub release asset digests](https://github.blog/changelog/2025-06-03-releases-now-expose-digests-for-release-assets/)
