# PMX

PMX is a Windows 11 live guitar workstation built around one simple rule: **the Sonicake Pocket Master is audio I/O only; PMX does the processing.**

```text
Guitar -> Pocket Master -> USB/ASIO -> PMX -> USB/ASIO -> Pocket Master -> headphones/speaker
```

## 0.2.0-alpha.2 scope

The first hardware-test alpha includes:

- Pocket Master ASIO device discovery and explicit channel/rate/buffer selection
- safe monitoring, Mute and FX Bypass
- Gate, Compressor, Drive, EQ, Chorus, Delay and Reverb
- Neural Amp Modeler `.nam` loading
- mono cabinet/body IR `.wav` loading
- chromatic tuner
- shared BPM / Tap Tempo / metronome
- one 120-second stereo looper with overdub and one-level Undo/Redo
- Quick Recorder to stereo WAV
- local preset saving/loading
- settings/diagnostics and driver-reported I/O latency
- GitHub Releases update checker with SHA-256 verification
- per-user Windows installer that preserves user data
- screenshot-matched PMX interface based on the supplied design

This is an **alpha**. A successful CI build is not the Pocket Master hardware acceptance test; use `docs/testing/first-pocket-master-test.md` on the actual Windows 11 + Pocket Master setup.

## Not in this first alpha

Bass, Synth, guitar-triggered Drums/Percussion, Piano and Violin remain committed PMX features. They are the next instrument-engine milestone after the live-audio foundation passes hardware testing. PMX Studio (the simplified DAW) is a later separate project.

## Build

Windows x64 is the supported target.

```powershell
cmake -S . -B build -A x64 -DPMX_BUILD_GUI=ON -DPMX_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The GitHub Actions workflow builds the unsigned test EXE and Inno Setup installer. The first alpha is intentionally unsigned; do not disable Windows security to run it.
