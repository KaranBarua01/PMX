# PMX 0.2.0-alpha.1 — First Pocket Master Hardware Test

This is a **hardware-test alpha**, not a production release. The test establishes whether the Windows/JUCE audio foundation works correctly with the Sonicake Pocket Master used only as a USB/ASIO interface.

## Before starting

1. Install the official Pocket Master Windows ASIO driver.
2. Connect guitar -> Pocket Master input.
3. Connect Pocket Master -> Windows 11 PC over USB.
4. Connect headphones/speaker to the Pocket Master output path you normally use.
5. Close DAWs or other applications that may have exclusive control of the Pocket Master ASIO driver.
6. Start with headphone/speaker volume low.

## Test sequence

### 1. First run and device detection

- Launch PMX.
- Press **Continue** on Welcome.
- Expected: PMX finds a Pocket Master ASIO device and shows Input / Output / ASIO ready.
- If it cannot open the device, PMX must show a readable error and must not silently use the laptop microphone or speakers.

### 2. Audio return and monitoring

- Press **Test Audio**.
- Strum the guitar.
- Expected: guitar reaches the Pocket Master output through PMX.
- Check carefully for a doubled dry signal. If direct hardware monitoring is also audible, document it before continuing.
- Confirm there is no feedback loop.

### 3. Bypass and mute

- Toggle **Bypass** with the UI and `B` shortcut.
- Expected: rack effects are bypassed but output protection remains active.
- Press `M`.
- Expected: the complete PMX output mutes/unmutes.

### 4. Guitar rack

- Enable/disable Gate, Compressor, Drive, EQ, Mod, Delay and Reverb.
- Open the Delay editor.
- Change time, feedback and mix.
- Use Tap Tempo.
- Expected: parameters change without crash, runaway feedback or obvious clicks.

### 5. NAM

- Open NAM/IR browser -> Import NAM.
- Select a known-working mono `.nam` model.
- Expected: the model loads, its file name appears on Live, and the guitar tone changes.
- Invalid/unsupported models must produce a readable error and leave PMX usable.

### 6. Cabinet IR

- Import a mono PCM16 or float32 `.wav` cabinet IR shorter than PMX's safe limit.
- Expected: the IR loads, its name appears on Live, and cabinet filtering is audible.
- Unsupported/malformed files must not replace the last working sound.

### 7. Tuner

- Open Tuner and play single notes including low E, A and high E.
- Expected: stable note name, frequency and cents display when confidence is sufficient; silence should show no fake note.

### 8. Presets

- Make a tone change and press **Save Preset**.
- Open Presets and reselect the saved tone/name.
- Expected: preset metadata survives app restart and missing asset files are reported rather than silently substituted.

### 9. Looper

- Record a short phrase.
- Play it back.
- Overdub once.
- Undo and Redo the overdub.
- Stop the loop.
- Save Loop and Export WAV.
- Clear the loop and confirm the destructive action.
- Expected: loop timing is stable, no feedback accumulation occurs, and exported WAV opens normally.

### 10. Metronome and tap tempo

- Tap a tempo using `P` or Delay Tap Tempo.
- Enable/use the metronome path.
- Expected: shared tempo stays consistent and does not interrupt guitar audio.

### 11. Quick Recorder

- Press `R` to start Quick Recorder.
- Play for at least one minute.
- Press `R` again to stop.
- Expected: a stereo WAV is written under `Documents\PMX Recordings`, with no audio-thread stall.

### 12. Buffer stability

Test only values the ASIO driver reports as supported.

- Start at **128 samples** and play/loop for at least 30 minutes.
- If available, test **64 samples** for lower latency.
- If crackles occur, test **256 samples**.
- Record the sample rate, buffer size, driver-reported I/O latency, and any dropouts/clicks.

### 13. Disconnect/reconnect

- While PMX is open, disconnect the Pocket Master USB cable.
- Expected: PMX mutes safely, reports disconnection, and does not switch to laptop audio.
- Reconnect and use Try Again / Apply Changes.
- Expected: the app reopens the device without needing a restart when the driver permits it.

## Pass criteria for the first alpha

The alpha passes its first hardware gate when all of these are true:

- clean Pocket Master input/output routing is established;
- no silent device fallback occurs;
- 128-sample operation is stable for 30 minutes on the user's system;
- bypass/mute, NAM, IR, tuner, presets, looper, metronome/tap and Quick Recorder work without crash;
- disconnect/reconnect fails safely;
- no critical doubled-monitoring or feedback-path issue remains.

Bass, Synth, guitar-triggered Drums, Piano and Violin are the next milestone after this hardware foundation is accepted.
