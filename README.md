# SQUIGGLE! 0.4.0 — Hybrid Synthesizer

Windows-first JUCE 8 / CMake VST3 + Standalone instrument. This development build is based **only on the supplied `HYBRID_SYNTH_v0_2.zip` source project**. It advances the original POLY/MONO foundation with a band-limited wavetable oscillator, a playable sample layer, a local sample library, and an interactive ADSR editor.

> Build status: source package prepared; it has **not** been compiled in this environment. A local CMake configure was attempted but could not fetch JUCE 8.0.6 because this environment could not resolve `github.com`. Use the included GitHub Actions workflow or a local Windows/Visual Studio 2022 environment with internet access to compile it.

## Included in 0.4.0

- 16-voice POLY mode and separate one-voice MONO mode.
- Sine, Saw, Square and Triangle wavetable families.
- Additive wavetable generation with harmonic-limited mip levels to reduce aliasing at higher notes.
- Wavetable selector, cross-morph control and independent wavetable level.
- Sample playback layer mixed simultaneously with the wavetable oscillator.
- WAV, AIFF, FLAC and OGG import through JUCE's basic audio formats (availability depends on JUCE build/platform codecs).
- Sample waveform display with selected Start/End region, plus Root Note, Loop and Level controls.
- Imported samples are copied to the local `SQUIGGLE/UserLibrary/Samples` folder under the user's application-data directory.
- Interactive ADSR graph: drag Attack, Decay, Sustain and Release handles; numeric controls remain linked to the same host-automatable parameters.
- ADSR time segments are displayed proportionally to their parameter times. The sustain hold uses a fixed visual reference width; it is not a fixed note duration.
- Per-module randomization for the envelope, wavetable and sample source, plus Randomize All for the currently implemented modules.
- APVTS host state save/restore, including the absolute path to the currently loaded sample.
- Resizable editor; two main tabs only: SYNTH and USER. Technical settings are kept separate from the main tabs and are not implemented yet.

## Build on Windows 10/11

Prerequisites:

- Visual Studio 2022 with **Desktop development with C++**.
- CMake 3.22 or newer.
- Git and an internet connection for the first configure, because CMake fetches JUCE 8.0.6.

Open **Developer PowerShell for VS 2022** in the project root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Expected outputs:

- VST3: `build\Squiggle_artefacts\Release\VST3\SQUIGGLE!.vst3`
- Standalone: `build\Squiggle_artefacts\Release\Standalone\SQUIGGLE!.exe`

The `.github/workflows/windows-build.yml` workflow performs the same configure/build on GitHub Actions and uploads the VST3 and Standalone files as an artifact.

## Install in FL Studio

1. Copy the complete `SQUIGGLE!.vst3` folder to `C:\Program Files\Common Files\VST3`.
2. In FL Studio, open **Options → Manage plugins** and run **Find installed plugins**.
3. Load **SQUIGGLE!** as an instrument and test MIDI notes, sample import, loop behaviour and ADSR release.

Imported samples are limited to the first 120 seconds per file to keep memory use bounded. The project uses a new product/bundle identity (`com.squiggle.synth`), so it can coexist with the earlier HYBRID SYNTH build during testing.

## Source layout

- `Source/PluginProcessor.*` — APVTS parameters, MIDI routing, sample loading/library, state and randomization.
- `Source/SynthVoice.*` — POLY/MONO voice, band-limited wavetable bank, sample playback and ADSR.
- `Source/PluginEditor.*` — SYNTH/USER tabs, waveform display and draggable ADSR editor.
- `CMakeLists.txt` — JUCE 8.0.6 project and VST3/Standalone targets.
- `STATUS.md` — implementation and verification register.

## Important limitations

Granular synthesis, CHROMA/pitch detection and spectral processing, Delay, Reverb, custom wavetable import, full preset browser, advanced USER library organization, and the Technical Settings DSP-quality modes are still pending. Sample paths stored in host state are absolute paths; moving a project to another computer requires copying the sample into that computer's SQUIGGLE! library and reloading it. No feature in this package should be considered FL Studio-verified until the generated VST3 has been built and tested there.
