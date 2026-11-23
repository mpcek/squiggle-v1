# SQUIGGLE! 0.4.0 — STATUS

## Verification level

- Base archive inspected: `HYBRID_SYNTH_v0_2.zip` from the user's Library.
- Source implementation: written and statically reviewed in this session.
- CMake configure: ATTEMPTED; stopped before generation because FetchContent could not resolve `github.com` to download JUCE 8.0.6 in this environment.
- Compiled: NO.
- Standalone run: NO.
- VST3 validator: NO.
- FL Studio: NOT tested in this session.

## Implemented in source (not yet compiled)

- Preserved the original separate 16-voice POLY and one-voice MONO synthesisers.
- Preserved APVTS host automation and XML state save/restore.
- Added a shared band-limited wavetable bank with Sine, Saw, Square and Triangle families.
- Added harmonic-limited table levels for improved high-note behaviour.
- Added wavetable selection, cross-morph and independent level.
- Added a sample playback layer mixed with the wavetable source.
- Added sample waveform display and Start, End, Root Note, Loop and Level parameters.
- Added import and local storage in the SQUIGGLE! User Library/Samples directory.
- Added host-state persistence of the loaded sample's absolute path.
- Added a draggable ADSR graph connected to the same host-automatable ADSR parameters as the numeric sliders.
- ADSR graph time axis now scales Attack, Decay and Release proportionally; sustain uses a fixed 250 ms visual reference segment.
- Added envelope, wavetable and sample-source randomization plus Randomize All for implemented modules.
- Added a SYNTH tab and a USER sample-library tab. Technical Settings remains separate and is explicitly marked as pending.
- Added GitHub Actions workflow for Windows VST3 + Standalone builds.

## Pending modules / work

- Granular engine and its discrete pitch intervals.
- CHROMA: pitch detection/quantization and spectral processing.
- Delay (Time, Feedback, Filter, Mix).
- Reverb (Filter, Pre-Delay, Decay, Mix).
- Custom wavetable import and USER wavetable library.
- Full preset save/load browser and portable sample-reference strategy.
- USER library search, categories, tagging and richer management.
- Technical Settings: Performance/Balanced/High Quality DSP modes and interface-size preference.
- More deliberate voice-stealing policy and mono legato/return-to-held-note behaviour.
- Audio-thread stress testing, denormal/performance profiling and parameter automation testing.

## Known caveats to check during the first Windows build

- JUCE 8.0.6 APIs and MSVC compilation have not been exercised in this environment.
- The sample buffer is published atomically; replaced buffers are retained until processor destruction to avoid freeing sample memory on the audio thread.
- Sample playback uses linear interpolation and is intended as a first functional engine, not a final high-quality resampler. Imported audio is limited to the first 120 seconds.
- Wavetable morph crossfades from the selected waveform to the next waveform in the selector order.
- The sample path in host state is absolute and is not portable across machines.
- `SQUIGGLE!` is a new plugin identity and may appear alongside the old HYBRID SYNTH entry in FL Studio.
