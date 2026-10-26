# AURUM CLIP — Phase 2A clipping

Independent C++20/JUCE audio plugin project in the GoldClip repository. AURUM CLIP is the provisional original product name; it is not affiliated with Schwabe Digital. Published Gold Clip behavior informs the roadmap, not a claim of copied DSP or sonic equivalence.

**Phase 2A adds real clipping to the tested routing foundation.** Off, Hard, Classic and Modern modes have distinct original transfer curves, a smoothed -36 to 0 dBFS clip ceiling, fixed-quality FIR oversampling and compensated dry/wet/bypass paths. Existing routing, automation, stereo waveform telemetry and saved UI size remain functional. Gold, Alchemy, Box Tone, selectable quality/LPF, LUFS, presets/A-B and installers belong to later phases; they are not exposed as working controls. This is an independent implementation, not a proprietary algorithm recreation.

## Products and formats

- **AURUM CLIP Master:** landscape 1200×600 editor.
- **AURUM CLIP Track:** compact 520×600 editor, shared DSP; no Master-only controls.
- Windows 10/11 x64: VST3 + standalone for each variant.
- macOS 11+ deployment target, Universal arm64/x86_64: VST3 + AU + standalone for each variant. Runtime compatibility must be tested on the actual oldest supported OS before release.
- Matched mono/stereo layouts only; native 32/64-bit floating point processing. Automated core routing cases cover 44.1/48/88.2/96/192 kHz; clipping tests also cover these rates.
- Original vector interface scales 80–175%, with saved host-state size. No borrowed logos or artwork.

## Audio behavior

Original input → split: dry receives latency compensation only; wet → Input Trim → oversampled Clipper → reconstruction → mixer. Combine paths → Output Trim → Output. Linked mode uses amplitude gains `wet=blend`, `dry=1-blend`. Unlinked mode permits additive dB gains. Delta outputs **aligned wet minus original dry** independently of normal mixer gains/mutes, followed by Output Trim. It affects rendered audio. Plugin bypass crossfades to delayed unity dry in 20 ms; host bypass immediately selects that same delayed dry path. Off preserves bit-exact delayed unity at neutral settings. The host receives the actual integer oversampling latency, constant across clipping mode changes at a given sample rate.

Fixed High uses 4× at 44.1/48 kHz, 2× at 88.2/96 kHz and 1× above 96 kHz. These are processing factors, not selectable quality modes. The FIR has passband/phase limitations, including fractional latency compensation. See [clipping design and measured boundaries](docs/CLIPPING.md). **The stage ceiling is not a final sample-peak or true-peak guarantee:** reconstruction, mixing and output gain can exceed it. There is no hidden postmix limiter.

Standalone JUCE input-feedback protection mutes input on first launch; use Options / audio settings to configure devices deliberately.

## Dependencies and reproducible setup

CMake ≥3.24, C++20, Git, Python 3.9+. Tested tool versions are recorded in validation reports. CI pins CMake 3.31.6 and Ninja 1.11.1.4. Windows requires Visual Studio 2022 Desktop C++ and Windows SDK; macOS requires Xcode/Command Line Tools with Universal SDK support and Ninja. Native CI never treats one OS as validation for another.

JUCE **9.0.4** is pinned by immutable commit in `cmake/Identity.cmake`. CMake FetchContent fetches it automatically. To use an existing checkout, set `JUCE_SOURCE_DIR` with the scripts or configure with `-DJUCE_SOURCE_DIR=/absolute/JUCE`. The checkout Git HEAD must match the pinned hash; uncommitted dependency edits must be reviewed separately. The offline **dsp-only** preset needs no JUCE or network. Central identity/version/plugin codes are in the same CMake file. These are development defaults; preserve plugin codes after sessions are created.

```sh
python3 -m pip install cmake==3.31.6 ninja==1.11.1.4
# Framework-independent tests:
cmake --preset dsp-only
cmake --build --preset dsp-only
ctest --preset dsp-only --verbose
# macOS native Universal build, tests, screenshots and binary staging:
bash scripts/build-macos.sh
# Or local pinned JUCE checkout:
JUCE_SOURCE_DIR=/absolute/JUCE bash scripts/build-macos.sh
```

```powershell
# Native Windows x64, Developer PowerShell:
python -m pip install cmake==3.31.6 ninja==1.11.1.4
./scripts/build-windows.ps1
# Optional local dependency:
$env:JUCE_SOURCE_DIR='C:/src/JUCE'
./scripts/build-windows.ps1
```

If a locally mixed CLT installation selects stale libc++ headers, see the environment-specific workaround in [reference/toolchain notes](docs/REFERENCES.md); native CI uses matched compiler/SDK environments.

Native scripts run DSP and host/state tests, render real editor snapshots, and copy compiled products into `dist/{Master,Track}/{VST3,AU,Standalone}` (AU on macOS only). `dist/manifest.json` records source commit, file sizes, SHA-256 hashes and available native header inspection. Ordinary builds are unsigned development artifacts. macOS staging applies ad-hoc development seals after all bundle metadata is generated, then verifies every architecture. This is a local integrity/loader requirement and uses no release credentials. No Developer ID, Authenticode or notarisation is claimed.

## Test and CI workflow

`.github/workflows/native.yml` runs native Windows/macOS jobs for pull requests against main, pushes to main and manual dispatch. It builds both variants, executes deterministic DSP and JUCE host integration tests, renders editor snapshots at 80/100/175%, runs pluginval strictness 5 for both VST3s and strict auval for both AUs, verifies macOS architectures and uploads successful native products. Failures upload diagnostic logs separately. It does not package or sign releases. Native validation tools are not substitutes for manual DAW/audio-device tests.

The DSP tests cover gain accuracy, mono/stereo independence, five sample rates, variable blocks, both sample precisions, true dry path, linked/unlinked mixing, mutes, Delta nulling, bypass, smoothing, invalid samples/parameters, allocation tracking, min/max envelopes and overflow. Host tests exercise parameter/state integration, schema migration/rejection, clipping controls, size recall and both sample precisions. Numerical tolerances live beside assertions. Current measurements are in [Phase 2A validation](docs/PHASE2-VALIDATION.md); [Phase 1 validation](docs/VALIDATION.md) is historical. See [architecture](docs/ARCHITECTURE.md) and [actual validation](docs/VALIDATION.md).

## Development workflow

`main` is stable. Phase 1 remains in PR #1; Phase 2A builds on it in `develop/phase-2-clipping`; completed modules are committed separately and reviewed through a pull request. Do not merge automatically. Preserve functioning code and stable state/parameter IDs. Build outputs are ignored. Dependency updates require a new reviewed pin and native validation.

## Roadmap and acceptance boundaries

1. **Foundation (PR #1):** project/variants, signal routing, parameters/state, telemetry, original functional editor, deterministic tests, native CI.
2. **Clipping and oversampling:** Phase 2A implements curves/Off, ceiling, fixed FIR quality, dry compensation and deterministic measurements. Phase 2B will add quality/filter selection, LPF and a broader alias/overshoot/CPU acceptance matrix. No true-peak guarantee without evidence.
3. **Gold:** distinct monotonic Modern/Classic families tied to ceiling, unity compensation, transfer/harmonic plots and legitimately obtained reference calibration.
4. **Alchemy and Box Tone:** Master-only dynamic frequency shaping and precision contour filters, response/phase/stereo validation.
5. **Product workflow:** calibrated loudness, clip clock using real transport, final waveform semantics and layout, trim linking/offset, presets and A/B, global preferences.
6. **Release:** manual multi-DAW tests, CPU/automation stress, installers, dependency notices, optional signing/notarisation in a separate credentialled workflow.

## Install/uninstall development binaries

Close hosts before copying bundles. Windows VST3: `%CommonProgramFiles%/VST3/`; macOS VST3: `~/Library/Audio/Plug-Ins/VST3/`; macOS AU: `~/Library/Audio/Plug-Ins/Components/`. Standalone apps can run from a development folder; configure audio devices through their Options menu. Unsigned/unnotarised builds may be rejected by OS security policy. No privileged installation or security-policy changes are performed by this repository. Remove only the copied AURUM CLIP bundles to uninstall. No installer/uninstaller exists yet.

## Licensing and references

[Dependency notices](docs/THIRD-PARTY-NOTICES.md) include the explicit JUCE licensing obligation. The owner has not selected a source distribution licence. [Official reference review](docs/REFERENCES.md) records the fetched manual's date and the unresolved version discrepancy in the supplied brief. No proprietary equations, brand assets or licensed reference audio are committed.
