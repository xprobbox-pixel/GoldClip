# AURUM CLIP — Phase 1 foundation

Independent C++20/JUCE audio plugin project in the GoldClip repository. AURUM CLIP is the provisional original product name; it is not affiliated with Schwabe Digital. Published Gold Clip behavior informs the roadmap, not a claim of copied DSP or sonic equivalence.

**Phase 1 is a working routing foundation, not the completed mastering processor.** It implements smoothed input/output trims, true original-input dry routing, linked linear Wet/Dry, independent dB Wet/Dry with mutes, actual Delta monitoring, bypass, peak/overload telemetry, real waveform envelopes, automation and validated state recall. No clipping, Gold, Alchemy, Box Tone, oversampling, LUFS, presets/A-B or installers are exposed as if implemented.

## Products and formats

- **AURUM CLIP Master:** landscape 1200×600 foundation editor.
- **AURUM CLIP Track:** compact 520×600 foundation editor, shared DSP; no Master-only controls.
- Windows 10/11 x64: VST3 + standalone for each variant.
- macOS 11+ deployment target, Universal arm64/x86_64: VST3 + AU + standalone for each variant. Runtime compatibility must be tested on the actual oldest supported OS before release.
- Matched mono/stereo layouts only; native 32/64-bit floating point processing. Automated core routing cases cover 44.1/48/88.2/96/192 kHz; 192 kHz support here describes Phase 1 routing, not future nonlinear algorithms.
- Original vector interface scales 80–175%, with saved host-state size. No borrowed logos or artwork.

## Audio behavior

Original input → split: dry stays unchanged; wet → Input Trim. Combine paths → Output Trim → Output. Linked mode uses amplitude gains `wet=blend`, `dry=1-blend`. Unlinked mode permits additive dB-domain gains; it can exceed full scale. Delta outputs **wet minus dry** independently of the normal mixer gains/mutes, followed by Output Trim. It affects rendered audio. Plugin bypass crossfades to unity dry in 20 ms; host bypass callback is immediate unity dry. There is no limiter, clipping ceiling or true-peak guarantee. Phase 1 reports zero latency because there are no filtering/delay stages. Standalone JUCE input-feedback protection mutes input on first launch; use Options / audio settings to configure devices deliberately.

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

Native scripts run DSP and host/state tests, render real editor snapshots, and copy compiled products into `dist/{Master,Track}/{VST3,AU,Standalone}` (AU on macOS only). `dist/manifest.json` records source commit, file sizes, SHA-256 hashes and available native header inspection. Ordinary builds are unsigned development artifacts; macOS toolchains may apply ad-hoc signatures. No Developer ID, Authenticode or notarisation is claimed.

## Test and CI workflow

`.github/workflows/native.yml` runs native Windows/macOS jobs for pull requests against main, pushes to main and manual dispatch. It builds both variants, executes deterministic DSP and JUCE host integration tests, renders editor snapshots at 80/100/175%, runs pluginval strictness 5 for both VST3s and strict auval for both AUs, verifies macOS architectures and uploads successful native products. Failures upload diagnostic logs separately. It does not package or sign releases. Native validation tools are not substitutes for manual DAW/audio-device tests.

The DSP tests cover gain accuracy, mono/stereo independence, five sample rates, variable blocks, both sample precisions, true dry path, linked/unlinked mixing, mutes, Delta nulling, bypass, smoothing, invalid samples/parameters, allocation tracking, min/max envelopes and overflow. Host tests exercise parameter/state integration, schema/variant rejection, size recall and both sample precisions. Numerical tolerances live beside assertions. See [architecture](docs/ARCHITECTURE.md) and [actual validation](docs/VALIDATION.md).

## Development workflow

`main` is stable. Phase 1 lives on `develop/phase-1-foundation`; completed modules are committed separately and reviewed through a pull request. Do not merge automatically. Preserve functioning code and stable state/parameter IDs. Build outputs are ignored. Dependency updates require a new reviewed pin and native validation.

## Roadmap and acceptance boundaries

1. **Foundation (this PR):** project/variants, signal routing, parameters/state, telemetry, original functional editor, deterministic tests, native CI.
2. **Clipping and oversampling:** separate Hard/Classic/Modern curves and Off, ceiling/LPF; prepared dry compensation; quality/filter selection; impulse, latency, alias and overshoot measurements. No true-peak guarantee without evidence.
3. **Gold:** distinct monotonic Modern/Classic families tied to ceiling, unity compensation, transfer/harmonic plots and legitimately obtained reference calibration.
4. **Alchemy and Box Tone:** Master-only dynamic frequency shaping and precision contour filters, response/phase/stereo validation.
5. **Product workflow:** calibrated loudness, clip clock using real transport, final waveform semantics and layout, trim linking/offset, presets and A/B, global preferences.
6. **Release:** manual multi-DAW tests, CPU/automation stress, installers, dependency notices, optional signing/notarisation in a separate credentialled workflow.

## Install/uninstall development binaries

Close hosts before copying bundles. Windows VST3: `%CommonProgramFiles%/VST3/`; macOS VST3: `~/Library/Audio/Plug-Ins/VST3/`; macOS AU: `~/Library/Audio/Plug-Ins/Components/`. Standalone apps can run from a development folder; configure audio devices through their Options menu. Unsigned/unnotarised builds may be rejected by OS security policy. No privileged installation or security-policy changes are performed by this repository. Remove only the copied AURUM CLIP bundles to uninstall. No installer/uninstaller exists yet.

## Licensing and references

[Dependency notices](docs/THIRD-PARTY-NOTICES.md) include the explicit JUCE licensing obligation. The owner has not selected a source distribution licence. [Official reference review](docs/REFERENCES.md) records the fetched manual's date and the unresolved version discrepancy in the supplied brief. No proprietary equations, brand assets or licensed reference audio are committed.
