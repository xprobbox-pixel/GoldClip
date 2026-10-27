# AURUM CLIP — GoldClip project

Independent C++20/JUCE stereo mastering processor, originally specified as a Gold Clip-inspired workflow. AURUM CLIP is original branding and independently designed DSP; it is unaffiliated with Schwabe Digital and makes no claim of copied algorithms or sonic identity. Version 1.0.0 has passed native Windows/macOS builds, automated plugin validation and installer tests as an unsigned development release; see [release validation](docs/RELEASE-VALIDATION.md) for actual results and limitations.

## Master and Track

**Master:** landscape 1200×600 editor, Gold, clipping, dynamic Alchemy, linear-phase FIR Box Tone, stereo input/output peak ladders, short-term LUFS, stage Clip Clock, waveform, independent wet/dry mixer, trim linking, presets and A/B.

**Track:** compact 520×600 editor sharing the Gold/clip/quality/routing engines. It omits Alchemy and Box Tone from both the editor and host parameter list. It also omits their FIR processing latency/CPU work. Independent wet/dry controls and Delta are available in Settings. Both editors scale 80–175%, including host dragging, keyboard-editable values, double-click/Alt-click defaults and named controls/tooltips.

Formats: Windows 10/11 x64 VST3 + standalone; macOS Universal arm64/x86_64 AU + VST3 + standalone. Matched mono/stereo, native float/double, tested sample-rate matrix 44.1/48/88.2/96/192 kHz. Deployment minimum macOS 11 is a build target; physical oldest-OS validation is distinct from compilation.

## Processing and controls

Original input splits into an untouched, delayed dry path and:

`Box Tone (Master) → Input Trim → Oversampling → Clipper → Alchemy (Master) → Gold → optional Clip Filter → Reconstruction → optional wet sample-peak guard → Mixer → Output Trim`.

- **Clipper:** Off, Hard, Classic, Modern; smoothed ceiling -36 to 0 dBFS. Classic/Modern are distinct smooth monotonic knees. Off leaves Gold/Alchemy usable and warns when held input exceeds the chosen ceiling.
- **Gold:** separate Modern (0–2.5 dB) and Classic (0–6 dB) parameter/curve families. Symmetric instantaneous low-level enhancement withdraws towards the ceiling. Classic curvature varies with amount. Unity compensates by the selected Gold dB amount independently of the mixer/trims.
- **Alchemy:** Master-only, 0–2 dB, stereo-linked amplitude modulation of a broad upper-frequency contour centered near 6.5 kHz, with high-frequency recovery. Powerful low-frequency events can modulate simultaneous highs. It is not a fixed EQ or conventional slow compressor.
- **Box Tone:** Master-only Flat/Classic/Modern, 129-tap symmetric FIR, measured subtle contours. Flat uses an exact delayed bypass. The fixed 64-sample tone delay remains in all Master modes to permit smooth switching.
- **Quality:** independent realtime/offline High 4×/2×, Pristine 8×/4×, Extra Pristine 16×/8× at 44.1–48/88.2–96 kHz. At 192 kHz the factors are 1×/2×/4×. Linear FIR and minimum-phase IIR are selectable. Six graphs are prepared outside the callback; only the active graph normally runs. New graphs warm, then crossfade over 50 ms. Fixed maximum PDC at a given rate/variant avoids host latency changes during switching. FIR integer compensation and IIR phase mean frequency-dependent dry/wet differences remain possible; Delta exposes the actual difference.
- **Clip Filter:** optional oversampled one-pole low-pass, 4–22 kHz setting capped at 45% of host sample rate. See measured response/limits; it is not an undocumented change to the clipper.
- **Wet guard:** explicit 32-sample lookahead, stereo-linked sample-peak gain control with immediate reduction / 20 ms release. Applies to reconstructed wet only when clipping is enabled; bypassing it releases smoothly. **It does not guarantee inter-sample true peaks or limit the final parallel sum.** Mixing, Output Trim and unprocessed dry can exceed the stage ceiling. Disable it to inspect raw reconstruction overshoot.
- **Mixer:** linked linear wet/dry blend, or independent -60 to +6 dB levels and mutes. Delta substitutes aligned wet minus original dry before final Output Trim.
- **Trims:** -12 to +12 dB. Link preserves the current sum when enabled from the UI; effective output is linked offset minus input. Shift-drag Input adjusts the offset. Settings provides exact offset entry; DAW automation can address Input and Offset explicitly. All audio controls are smoothed. Plugin bypass crossfades over 20 ms; host bypass immediately returns the same delayed unity dry input.

The standalone exposes JUCE audio-device settings through its Options menu. Input is muted on first launch to prevent unexpected microphone-to-speaker monitoring. Deliberately select devices and enable input. No processing changes based on offline rendering except the explicitly selected offline oversampling quality. Offline quality changes on an already-running instance use the documented warmed crossfade; a freshly prepared render uses its selected graph from the first block.

## Metering, transport and display

Short-term loudness uses BS.1770 K weighting, mono/stereo channel-energy weighting and a full 3-second window. Startup/reset displays `—`; valid silence displays `−∞`. This is ungated short-term LUFS, not integrated programme loudness. Input is measured on the original compensated input; output after the final mix/trim. Measurement continues on actual callbacks with the editor closed and while transport is stopped; no synthetic values or transport-time advance is invented.

Clip Clock holds the largest measured transfer-stage attenuation and DAW timeline seconds where playing transport supplies them. Standalone/stopped/missing transport shows `—` for time. It persists across stops/loops until Reset. The event is timestamped at the oversampled processing position; filter pipeline delay gives a small, documented timing offset, particularly with minimum phase. It is a stage diagnostic, not an exact sample locator or output true-peak meter.

Meter scales +6/−36, +4/−24 and +2/−12 dBFS cycle by double-clicking the displayed scale or through Settings. Grey waveform envelopes are actual original input; dark/gold contours are actual processed output; blue guides are the stage ceiling. Peak-preserving 64-sample envelopes feed a bounded lock-free queue. History is 25,600 samples, not a fixed number of seconds. Waveform Off stops graphical capture; meters/analysis continue. Reset clears held peaks, overload, Clip Clock and loudness startup. Colour warnings also include text.

## Presets, A/B and persistence

Four factory presets are actual parameter states. User presets save atomically under the OS user application-data `AurumClip/Master` or `Track` directory. Header arrows navigate; Save creates/replaces a named user preset; Settings renames/deletes user presets or makes the current state the default. A/B initially follows the current edits; selecting the other state establishes independent memories. Copy works in both directions. Both memories, active state and tracking status persist in DAW state and user presets.

Stable APVTS IDs and schema 3 validate complete transactions, finite values, variants and future versions. Schema 1/2 sessions migrate with new processing disabled/defaulted. Cross-variant snapshots are rejected. UI size is saved in host state and preferences; tooltips and meter scale have persistence. No audio or telemetry is stored externally. WAV/CSV exports exist only in explicitly invoked tests/measurement tools.

## Build and test

Requirements: native C++20 compiler/SDK, CMake ≥3.24, Git; Python 3.12 for the full plotting workflow; Ninja on macOS; Visual Studio 2022 Desktop C++ on Windows; Inno Setup 6 for Windows packaging. CI pins CMake 3.31.6, Ninja 1.11.1.4, NumPy 2.2.6 and Matplotlib 3.10.3. JUCE 9.0.4 is pinned by immutable commit in `cmake/Identity.cmake`. CMake fetches it automatically, or accepts `JUCE_SOURCE_DIR` at that exact Git revision. The framework-independent core tests need no JUCE/network.

```sh
python3 -m pip install cmake==3.31.6 ninja==1.11.1.4 numpy==2.2.6 matplotlib==3.10.3
cmake --preset dsp-only
cmake --build --preset dsp-only
ctest --preset dsp-only --verbose
# Native macOS Universal: compile, test, render UI and measure DSP, stage binaries:
JUCE_SOURCE_DIR=/absolute/pinned/JUCE bash scripts/build-macos.sh
python3 scripts/plot-measurements.py dist/measurements
bash scripts/package-macos.sh
```

```powershell
# Native Windows x64 Developer PowerShell:
python -m pip install cmake==3.31.6 ninja==1.11.1.4 numpy==2.2.6 matplotlib==3.10.3
$env:JUCE_SOURCE_DIR='C:/src/pinned-JUCE' # optional; unset to fetch
./scripts/build-windows.ps1
python scripts/plot-measurements.py dist/measurements
./scripts/package-windows.ps1
```

Do not point JUCE_SOURCE_DIR at another revision. Toolchain-specific CLT/libc++ notes are in [references](docs/REFERENCES.md). Native CI runs tests, pluginval, strict auval, architecture checks, measured plots, installers and ephemeral install/uninstall checks. A build on one OS does not validate the other. Manual DAW/hardware/oldest-OS checks are listed separately in the release report.

Measurement exports: run `AurumHostTests --measure /absolute/directory`, then the plot script. CSVs contain actual transfer, harmonics, Box Tone, Alchemy, alias and timing results. Sine, deterministic drum, synthetic mix and noise WAVs are generated test material, not commercial reference recordings. `scripts/compare-renders.py reference.wav candidate.wav --output comparison.json` optionally aligns lawful same-input renders and reports gain/residual/peaks; it requires FFmpeg and NumPy. No proprietary reference audio was supplied or sonic calibration claimed.

## Installers and release status

See [packaging instructions](packaging/README.md) for exact paths, selectable components, uninstall and optional signing/notarisation secrets. Ordinary CI produces unsigned Windows EXE and ad-hoc-development macOS PKG installers. Signing is a separate manual workflow; no credentials are committed. Missing credentials do not prevent development installers but do prevent a signed/notarised release claim.

Build outputs are under `dist`; release deliverables include source archives, binaries, installers, per-file SHA-256 manifests, logs and validation reports. No AAX is built without an authorised SDK. No Linux build is used to certify native outputs.

## Licensing and development

JUCE licensing must suit the intended distribution. [Third-party notices](docs/THIRD-PARTY-NOTICES.md) document dependencies. The owner has not chosen a public source-distribution licence. This repository includes no copied protected artwork, proprietary DSP code or original reference audio.

`main` remains stable; development changes use PRs and descriptive commits. Prior phase reports are historical. The final product's equations, limitations, tolerances, actual validation and signing status must be reviewed before any commercial production-ready claim.
