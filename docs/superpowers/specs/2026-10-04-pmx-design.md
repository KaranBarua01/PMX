# PMX: Windows Live Guitar Workstation

**Design date:** 2026-10-04  
**Repository:** KaranBarua01/PMX  
**Status:** Written design for owner review. This document is not an implementation or a released installer.

## 1. Purpose and agreed boundaries

PMX is a simple Windows 11 application for playing guitar through effects, making instrument-like sounds, and building loops. The user should be able to install it, select the audio device, choose a sound, and play without learning a DAW or using Git commands.

The Sonicake Pocket Master is strictly the USB audio interface. PMX must not send MIDI, edit the device, change its presets, update its firmware, or depend on its footswitches. All PMX processing and loop controls live on the PC. A one-time manual clean/bypass configuration on the Pocket Master may be necessary for its audio routing; this is not automated by PMX.

The agreed long-term instrument set is guitar, bass, synth, drums, piano, and violin. A future Studio workspace can reuse the engine, but multitrack recording and arrangement are not part of this build.

**Success:** a usable sound-and-loop application with reliable audio, understandable controls, locally saved work, and an Update button. A rendered interface or a successful compilation alone does not establish that success.

## 2. Delivery boundary: a working foundation before more instruments

The product will be delivered in testable increments, not as one unverified collection of features.

| Increment | Included | Release gate |
|---|---|---|
| Foundation: first usable test build | Windows audio setup; guitar effects; NAM and IR import; looper; presets; local loop save/export; installer and Update button | Windows build, automated tests, and a Pocket Master listening/routing test |
| Instrument expansion | Single-note guitar-controlled bass and synth; calibrated note/onset-to-drum triggering; metronome and a small beat sequencer | Tracking, latency, false-trigger and practical playing tests |
| Sampled instruments | Piano and violin using original or redistribution-permitted assets | Auditioned sound quality, note handling, licence checks and performance tests |
| Future separate project | PMX Studio recording/arrangement workspace | A separate design; not a prerequisite for the live app |

Piano, violin, bass, synth, and drums remain committed product goals; they are not silently removed. Their controls are not presented as finished features in the foundation build. Experimental builds must label unfinished modes explicitly.

The first candidate will use an alpha version such as `0.2.0-alpha.1`, not a misleading production `1.0`. Version 1.0 requires the accepted live-app scope to pass its release gates. No fixed completion date is promised by this design.

## 3. Technical decisions

Use a native C++20/JUCE standalone application, CMake builds, and Windows x64 distribution. Retain JUCE 9.0.3 as the initial candidate dependency, pinned to commit `be29c81492b6151c8ea8d14c840e1311963b3a83`. The official 9.0.3 release exists; pinning this version does not imply that the old PMX prototype has passed a Windows build. [1]

Use the Pocket Master manufacturer's ASIO driver already installed on the PC. Do not bundle or silently install third-party drivers. Enumerate the driver and channel names; do not hard-code an unverified device string. Other enumerated devices can appear in settings, but only Windows 11 with the Pocket Master is the acceptance target.

Use NeuralAmpModelerCore for NAM inference rather than trying to reverse-engineer the Pocket Master's firmware. It is an upstream C++ DSP library with model-loading and benchmarking tools. Its exact commit and tested model formats must be recorded before integration is released. [2]

Use an installer rather than requiring a source build. The proposed packaging tool is Inno Setup in per-user mode. Its `PrivilegesRequired=lowest` setting supports non-administrative installation. Packaging-tool and dependency notices must accompany distributions. [3]

JUCE offers AGPLv3 and commercial licensing paths. The proposed public-source distribution path is AGPLv3-compatible, with corresponding source and third-party notices for every binary release. This design does not itself relicense old prototype code or accept commercial terms on the user's behalf. Verify the exact JUCE, ASIO integration, NAM, and packaging dependency terms before publishing binaries; incompatible terms block a release. Do not purchase licences without approval. [4]

## 4. Audio routing and component boundaries

### Foundation signal path

```text
Guitar
  -> Pocket Master audio input
  -> USB / ASIO input channel selected in PMX
  -> input trim -> gate -> compressor -> drive
  -> NAM (optional) -> cabinet IR (optional)
  -> EQ -> chorus -> delay -> reverb
  -> looper capture / playback mix
  -> master level -> output protection
  -> USB / ASIO stereo output
  -> Pocket Master headphones or speaker connection
```

Stereo spatial effects follow the mono guitar-processing stages. A loop captures the processed sound before master level and output protection. Playback rejoins after the live effects, so changing a live preset does not turn an already-recorded guitar loop into a different instrument or run it through the effects twice.

### Responsibilities

| Component | Owns | Must not own |
|---|---|---|
| Audio device layer | ASIO connection, supported rates/buffers, channels, reconnect state | Presets, downloads or instrument selection |
| Processing engine | Block processing, parameters, module bypass, graph state | Windows, file dialogs or network calls |
| Guitar rack | Gate, compressor, drive, NAM, IR, EQ and spatial effects | Device control |
| Instrument engine, later | Clean-input analysis and instrument generation | Audio hardware and update installation |
| Looper | Loop transport, audio buffers, overdub and undo state | Direct file writes inside the callback |
| Preset/asset store | Versioned settings, local file validation, backups | Audio-thread mutation of large objects |
| User interface | Controls, readable status, user commands | Ownership of live audio buffers |
| Update service | Release lookup, verified download and installer handoff | Real-time processing or firmware operations |

The processing engine exposes preparation, block-processing, reset and command/parameter boundaries without depending on the UI. A future Studio host can reuse those boundaries. This is not a promise of a zero-change DAW conversion.

## 5. Audio safety and performance contract

Start with monitoring muted and a conservative master level. Let the user explicitly enable monitoring after selecting input and output. Do not fall back silently to the laptop microphone or speakers if the Pocket Master disappears.

Request 44.1 kHz and 128 samples when the selected ASIO driver offers them. Offer only supported alternatives; 64 and 256 samples are test candidates, not guaranteed device capabilities. Show the actual selected configuration. Sample-rate changes require stopping audio and handling an existing loop before reinitialisation.

Read driver-reported input and output latency separately. Report their sum as **driver-reported I/O latency**, with known processing delay shown separately. Never label the buffer duration, CPU meter, or a guessed number as measured end-to-end latency. JUCE exposes input/output latency queries; physical round-trip validation is a separate test. [5]

No locks that can block, heap allocation, buffer resizing, full-loop clearing/copying, disk access, network requests, or UI calls are permitted in the audio callback. Preallocate working buffers and use bounded commands. Prepare model/IR replacements off the audio thread, swap only at a safe boundary, and reclaim old objects off that thread. Scalar changes are smoothed where necessary to avoid clicks.

Provide separate **FX bypass** and **Mute** controls. FX bypass skips the live rack, but retains master protection and any playing loop; Stop remains the way to stop the loop. Mute silences the entire output. This is software bypass, not a hardware true-bypass claim.

Output protection must prevent non-finite samples from reaching the device and limit overload. It is not protection against physically excessive headphone volume. Losing the device mutes output, stops transport, preserves available loop state, and offers an explicit reconnect action.

The first hardware test must establish a clean USB send, an audible USB return, and a monitoring arrangement without unwanted doubled dry guitar or a feedback loop. PMX cannot disable a hardware monitoring path by itself. If that routing cannot be established, document the limitation before claiming interface-only operation works.

## 6. Guitar rack and file imports

The rack has a fixed, understandable order in the foundation release. Each module has an on/off switch and a small set of controls; detailed controls sit behind an Advanced disclosure.

| Module | Controls shown with real units |
|---|---|
| Input/gate | Input dB, threshold dB, attack/release ms |
| Compressor | Threshold dB, ratio, attack/release ms, makeup dB |
| Drive | Drive amount, tone, output dB |
| NAM | File/name, input trim dB, output trim dB, compatibility status |
| IR | File/name, low-cut Hz, high-cut Hz, output dB |
| EQ | Low/mid/high gain dB with frequency controls in Hz |
| Chorus | Rate Hz, depth, mix percent |
| Delay | Time ms, feedback percent, mix percent; tap tempo |
| Reverb | Size/decay using the actual algorithm's units, damping, mix percent |
| Master | Output dB, level/clip indicators, Mute |

Never present a frequency control as a 0-100 value merely because another effect uses that scale. UI values, preset values, and DSP conversions must agree.

NAM and IR imports use the original local `.nam` and `.wav` files. Importing them into the Pocket Master does not also import them into PMX. The app does not extract profiles from the pedal and does not download paid or login-protected models automatically.

Validate model architecture/configuration, model sample rate, finite data, file size and processing cost. Do not identify compatibility from filenames or promise every NAM A1/A2 model works. Publish a tested compatibility table for the pinned inference engine. Use a verified, accounted-for resampling path when model and device rates differ, or reject the combination with a readable explanation; do not run a model at an arbitrary wrong rate.

Decode and prepare IRs outside the callback. Accept mono cabinet/body IRs in the foundation release, with safe sample-rate conversion and explicit duration limits in the UI. Reject unsupported stereo or malformed files rather than silently changing them. A failed import leaves the previous working rig intact. Avoid an abrupt uncabbed, high-gain output during replacement.

Presets keep module state and asset references, not just a preset name. Missing assets produce a repair prompt and keep monitoring muted until acknowledged. No third-party NAMs, IRs, or sample libraries are bundled without an appropriate redistribution licence. Starter sounds must work without requiring commercial assets.

## 7. Simple interface

Use five destinations: **Live**, **Looper**, **Presets**, **Settings**, and **Update**. Start at Live. Keep the top-level workflow usable without Advanced panels.

**Live:** selected sound, input/output meters, input level, main rack switches, master level, FX bypass, Mute, and compact loop transport. The foundation build shows Guitar; later validated instrument modes join this selector.

**Looper:** prominent Record/Play/Overdub, Stop, Undo/Redo, Clear, loop level, elapsed/total time, progress, Save Loop and Export WAV. Display state in text as well as visually. Keyboard shortcuts must not trigger while typing in a name or dialog.

**Presets:** Save, Save As, Rename, Duplicate, Delete, Import and Export. Warn before discarding an edited rig. Factory starter presets are read-only templates; user presets are editable. Do not impose the Pocket Master's 50-slot or five-model hardware limits on PMX.

**Settings:** device/ASIO control panel, input channel, output pair, rate/buffer, monitoring test, local storage, diagnostics and update preferences. Show a readable device-missing screen instead of an unexplained silence.

**Update:** installed version, available version, release notes, Check, Download, Install and Restart. It remains separate from the sound controls.

Support a resizable window, keyboard access and Windows display scaling. Do not display fabricated CPU or latency figures. CPU load is labelled audio-engine load, not total PC usage.

## 8. Looper behaviour

The foundation looper is one stereo loop, up to 120 seconds, with multiple overdubs and one-level undo/redo of the most recent overdub. It is not a multitrack recorder.

States are Empty, Recording, Playing, Overdubbing and Stopped. The main button cycles Record -> Play -> Overdub -> Play; from Stopped it resumes playback from the start. Stop finalises an active recording/overdub and stops playback. Clear requires confirmation for nonempty audio and returns to Empty. Mute does not erase a loop.

The first recording establishes loop length. At the limit, recording becomes playback and the UI explains why. Overdubbing must not feed playback back into the recorder as an additional input. Record only the new live signal into the overdub contribution.

Undo/redo operates on a completed overdub while Playing or Stopped, and is disabled during active recording/overdubbing. A new overdub invalidates the redo of an undone pass. Do not run a large snapshot copy or clear on the audio thread. Loop boundary smoothing and explicit headroom prevent avoidable clicks and overload; do not normalise or hard-clip every stored sample as a substitute for level management.

For the foundation, Save Loop/Export requires Stopped state. The worker writes a stable buffer snapshot, leaving audio playback responsive. Export 24-bit stereo PCM WAV at the session rate; save an internal lossless float WAV plus metadata for reopening. Existing filenames require confirmation before replacement. Disk-full, cancelled, or failed writes do not mark the loop as saved.

No quantisation or time stretching is promised in this release. Device-rate changes with a loop require Save/Discard/Cancel before reinitialisation. Preset changes do not clear a loop; closing or installing an update prompts for an unsaved loop.

## 9. Local data and privacy

Install application files separately from user data, for example under `%LOCALAPPDATA%\Programs\PMX`. Keep settings, versioned preset JSON, managed imported assets and update staging under `%LOCALAPPDATA%\PMX`. Use the Windows Documents known-folder location for user-selected loop exports, not a hard-coded username.

Preset records have a schema version, stable ID, display name, module parameters, bypass states and local asset hashes/references. Application settings separately hold the audio-device configuration. Loading a sound preset must not unexpectedly switch the audio device or global master safety state.

Use temporary files and atomic replacement for settings/presets, with a known-good backup. Back up metadata before migrations. Refuse a newer unsupported schema rather than overwriting it. Maintain independent user data so an installer cannot remove recordings or user presets.

Normal audio processing, looping, preset use and local instrument assets work offline. Only update checks/downloads need the network. No account, telemetry, audio upload, automatic cloud sync or embedded GitHub token is required. Diagnostics are local and shared only by an explicit export action, with sensitive paths removed.

## 10. Update design: no Git commands for the user

Use GitHub Releases in this repository as the distribution source. GitHub's public release API can be queried without authentication and exposes release assets. Asset digests are available for download integrity checks. [6][7]

**Checks:** perform a nonblocking startup check, at most once in 24 hours, and provide a manual Check button. The setting can be disabled. Never download or install just because the app starts. A failed check must not stop music-making.

**Channels:** stable builds ignore drafts/prereleases. Alpha builds use an explicitly labelled Preview channel so future alpha updates can be found; preview selection is visible and configurable. Compare parsed semantic versions, not strings or only publication dates. Follow pagination when finding a channel-compatible release.

**Download:** show release notes, version and size, then request user action. Select only the expected Windows x64 installer asset for this repository. Require HTTPS, validate redirect destinations, and verify size and SHA-256 against release metadata before execution. A missing digest, malformed version, unexpected asset, truncated file or mismatch blocks installation. Keep staged files out of the source/preset directories.

**Install:** the user presses Install and Restart. Block during recording, overdubbing or playback; offer to stop and save first. Back up settings, finish pending writes, close audio, and hand off to the installer. Do not kill a live take or try to overwrite the running executable from the audio process. Installer completion offers restart, then PMX restores settings with monitoring muted.

The first installer may be unsigned, with that fact clearly disclosed; do not describe it as verified by Microsoft or instruct users to disable Windows security. A checksum validates bytes relative to the trusted GitHub release, not an independent publisher signature or immunity to a compromised repository. A publisher-signing option can be added separately when available.

Keep the previous verified installer and a versioned settings backup. A failed download leaves the running version unchanged. A failed install offers recovery with the previous installer; do not claim automatic rollback until it is implemented and tested. Uninstall and update operations must preserve user-created files by default.

No installer is offered merely because a commit exists. Only a published release with passing required checks and a valid asset is eligible.

## 11. Reuse, testing and release evidence

The existing `PMX_v0.1_source.zip` is reference material, not a verified Windows product. Its status notes report three small core tests and explicitly say the Windows/JUCE executable was not built. Recheck those claims rather than treating previous chat descriptions as acceptance evidence. Review its audio-thread behaviour, state ownership and test coverage before reusing modules.

The repository must keep these evidence categories separate:

| Evidence | What it establishes |
|---|---|
| Automated unit/DSP tests | Particular behaviours on defined inputs |
| Windows CI build | Source can produce the Windows application |
| Installer/update test | Install, upgrade and recovery paths work on a Windows test system |
| Pocket Master hardware test | Actual routing, listening quality and usable latency on the user's setup |

Foundation release tests cover:

- Empty input, extreme/invalid samples, variable blocks and device loss without crashes or unsafe output.
- Correct channel selection; supported rate/buffer changes; no silent microphone fallback.
- Every looper transition, zero/short/max-length recordings, overdub isolation, undo/redo, preset switching and export failure.
- Preset round-trip, missing assets, invalid NAM/IR inputs, unsupported architectures, sample-rate mismatch and cancelled imports.
- Update up-to-date/available/offline/no-release responses, preview filtering, semantic ordering, digest mismatch and interrupted downloads.
- Installer upgrade over a prior build with existing presets and saved loops; unsuccessful upgrade and recovery.
- At least 30 minutes of hardware playback/looping at a supported stable buffer, followed by reconnect testing and listening for clicks, dropouts and doubled monitoring.

Release tests must stay effective in Release configuration; assertions compiled out by `NDEBUG` are not sufficient. Dependency versions, checksums, build commands and test results accompany each release. Windows CI cannot replace the user's physical-device test.

## 12. Later instrument-engine contract

Analyse the clean guitar input before nonlinear guitar effects. Keep pitch/onset analysis and sound generation separate so an instrument can be replaced without rewriting the app.

Start with monophonic notes, configurable gate/sensitivity, stable note transitions, retrigger control, pitch-bend smoothing and an All Notes Off action. Piano/violin tone quality depends on the actual engine/assets; do not label a simple waveform as a realistic sampled instrument.

Drums trigger actual synthetic or licensed sample sounds from calibrated notes/attacks, rather than claiming compression converts guitar audio into drums. Do not promise identification of the physical string from a mixed pickup signal when different strings can produce the same pitch. Chord-to-piano transcription, automatic articulation recognition and studio-quality violin are not initial acceptance requirements.

Measure pitch-detection/generation delay separately from ASIO I/O delay. No universal 5-15 ms guitar-to-instrument latency promise applies, especially for low notes or ambiguous attacks. Include dry/wet control so the original guitar can be mixed deliberately rather than leaking through unnoticed.

## 13. Explicit non-goals

No Pocket Master editor, firmware modifications, MIDI/Bluetooth integration, phone app, web audio frontend, multitrack timeline, piano roll, notation, VST host, plugin marketplace, cloud account, paid subscription or automatic model downloads in the foundation build. An extensible design must not turn these non-goals into mandatory dependencies.

## 14. Review checkpoint

This document records the selected product direction and defines the first usable increment. The next artifact is a task-by-task implementation plan for the foundation, with test cases and repository changes. Product code and release publication follow review of the written specification and that plan.

## Primary references

These references support dependency/API facts. Product behaviours, thresholds and release gates above are PMX design requirements, not claims that those features already exist.

1. [JUCE 9.0.3 release](https://github.com/juce-framework/JUCE/releases/tag/9.0.3) - verified release and candidate commit.
2. [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) - C++ inference library and test/benchmark tools.
3. [Inno Setup: PrivilegesRequired](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm) - per-user installer setting.
4. [JUCE licensing FAQ](https://juce.com/get-juce/) - open-source and commercial distribution paths.
5. [JUCE AudioIODevice](https://docs.juce.com/master/classjuce_1_1AudioIODevice.html) - device latency and control-panel APIs.
6. [GitHub Releases REST API](https://docs.github.com/en/rest/releases/releases) - public releases, channels and asset metadata.
7. [GitHub release asset digests](https://github.blog/changelog/2025-06-03-releases-now-expose-digests-for-release-assets/) - SHA-256 release asset integrity.
