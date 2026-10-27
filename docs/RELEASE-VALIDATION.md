# AURUM CLIP 1.0.0 — development release validation

Recorded 27 October 2026. Master and Track are implemented and packaged. These are unsigned development installers, not signed commercial releases. DSP is independently designed; no proprietary reference algorithm or sonic identity is claimed.

## Exact build provenance

- Repository: https://github.com/xprobbox-pixel/GoldClip ; implementation PR: https://github.com/xprobbox-pixel/GoldClip/pull/3 . Main has not been merged automatically.
- Tested development head: `c7efe0939fc0e84a2098d8550a6f3f63ec373eec`.
- Native CI run 18: https://github.com/xprobbox-pixel/GoldClip/actions/runs/40423782610 . Windows job `121924733491`, macOS job `121924733171`: both completed successfully.
- CI compiled PR merge commit `9ca5e42a51b4e55b6fb646b9439d2349c56bc187`. Its complete source tree equals the tested development head: `c268caaa252d478b971cc79ec2ddf89a46897db8`. Final documentation updates do not change these binaries.
- JUCE 9.0.4, pinned commit `3fb5cbc0340758596dac39f7c8de47d562b4896e`; C++20, Release builds.

## Native platform results

| Check | Windows 2022 runner | macOS 14 runner |
|---|---|---|
| Master and Track binaries | x64 VST3 + standalone built | Universal arm64/x86_64 VST3 + AU + standalone built |
| Core DSP tests | 1,559 checks, zero failures | 1,559 checks, zero failures |
| Host/state/preset/UI interaction tests | 475 checks, zero failures | 475 checks, zero failures |
| Host suite plus six editor captures | 481 checks, zero failures | 481 checks, zero failures |
| VST3 validation | Both variants, pluginval 1.0.3 strictness 5 passed | Both variants, pluginval 1.0.3 strictness 5 passed |
| Audio Unit validation | Not applicable | Both variants, strict auval passed |
| Installer generated | Inno Setup EXE | Apple PKG, six selectable components |
| Actual installer exercise | Install, compare payload hashes, uninstall passed | Install, compare payload hashes/seals, remove bundles/receipts passed |
| Distribution signing | No Authenticode | Ad hoc bundle seals only; PKG unsigned, no Developer ID/notarisation |

The installers were exercised on disposable native CI runners, not on the user's workstation. Windows runtime payloads were separately checked as PE x64. All six downloaded Mac executable bundles were separately verified to contain both architectures and pass strict all-architecture code-seal verification. This verifies universal contents, not physical testing on every Intel Mac or minimum OS.

## DSP and state coverage

Tests use deterministic generated signals and check native float/double, mono/stereo, rates 44.1/48/88.2/96/192 kHz and variable blocks including 1, 17, 257 and 4,096 samples. Coverage includes compensated neutral identity, host/plugin bypass, trim/mixer/Delta routing, anti-phase material, NaN/Inf handling, declared latency, all quality/phase paths and transitions, original clip curves, Gold gain/Unity/monotonicity/ceiling anchoring/curvature, linked wet guard, Box Tone, dynamic Alchemy and K-weighted three-second loudness.

Host tests cover parameter schema validation and migration, variant rejection, corrupt/future state, factory and user presets, save/load/rename/delete, default preset identity and malformed-default fallback, both A/B memories, UI size restoration, quality automation, transport/stopped Clip Clock behavior, reset and bypass. Actual editor double-click events distinguish clock reset from meter-scale changes. A C++ new/new[] allocation probe checks prepared quality transitions; it is not a universal malloc audit. Earlier local ASan/UBSan core testing passed all 1,559 checks; logs are retained separately from final native CI.

Numerical tolerances are in executable test assertions. Examples: Gold bounds and monotonicity use 1e-14 double tolerances, low-level gain/Unity 1e-6, linked anti-phase residual 1e-10 and wet-guard sample bound 1e-12. Tests assert behavior, not formal product certification.

## Measurements actually executed

Both native jobs exported transfer curves, amplitude sweeps, harmonic spectra, three coherent high-frequency alias probes, Box Tone, dynamic Alchemy, Clip Filter, 60 full Master rate/channel/quality/phase performance trials, and four five-second synthetic audio pairs. Plots and CSV/WAV data are included in each artifact. Performance figures are runner-specific and are not guarantees of DAW instance counts or audio-buffer deadlines.

Each final native artifact's eight input/output loudness readings was independently compared locally with installed FFmpeg 5.0.1 ebur128. All 16 comparisons passed the 0.05 LU tolerance; maximum absolute difference was 0.0005 LU. This validates the last complete three-second window of these signals, not integrated loudness or universal standards compliance.

At 48 kHz, FIR oversampling's measured third-harmonic alias reduction against the 1x reference across the selected 9/10/11 kHz probes was approximately 30.6–54.2 dB (High), 42.7–67.7 dB (Pristine) and 56.2–69.2 dB (Extra). These are selected coherent probes, not broadband alias guarantees or a promise that every higher quality setting wins at every frequency. The one-pole Clip Filter measured approximately -2.98 dB at the 8 kHz cutoff. Flat Box Tone retains delayed identity. Filter response and dynamic Alchemy measurements are provided for inspection rather than described as copies of another product.

## Visual inspection

Final native rendered Master and Track captures at 80%, 100% and 175% were inspected on both operating systems: twelve captures total. No essential control overlap or clipping was observed. Secondary text is intentionally compact at 80%; use larger scaling for easier reading. These captures use generated real audio, not decorative meter activity. They are editor rendering checks, not manual tests in a commercial DAW.

## Artifact integrity

Downloaded ZIP SHA-256 values matched GitHub's artifact digests. Every payload listed in the final native manifests was size/hash verified: 55 Windows entries and 77 macOS entries. Release-level manifest and SHA256SUMS provide installer/archive integrity and exact build provenance.

- Windows ZIP: `23d0d398372f99b3a19d766ea0235798c34d8feecdc3c196cceab6c5779bd926`
- macOS ZIP: `eb7881942448dc86342dbfb80ceec62533401fec8ef129eca19b51879f4f84c9`

Older run 13 exposed relocatable Mac installer placement. Component plists were changed to nonrelocatable standard destinations; run 14 and final run 18 passed installation and payload checks. Historical logs may describe older counts and must not be confused with final run 18 evidence.

## Remaining external validation and release work

Manual DAW listening, automation/render round trips in commercial DAWs, hardware device testing, physical Intel Mac execution of this final build, oldest supported OS testing and long-duration CPU/soak tests were not completed. The standalone was launched locally and exposed Options/audio-device settings with input muted to avoid feedback; live hardware monitoring was not validated.

No lawful proprietary reference recordings were supplied, so no sonic-match claim is made. The wet reconstruction guard bounds reconstructed wet sample peaks only; final mixing/output gain can exceed the clip ceiling and this is not a true-peak limiter. Clip Clock timing follows host transport with processing/filter alignment limitations. See ARCHITECTURE.md for equations, routing and these conventions.

Signing credentials were unavailable. Authenticode, Developer ID Application/Installer, notarisation, stapling and signed package assessment remain unexecuted. Optional signing workflows/scripts exist separately. Select final manufacturer/bundle identities, confirm JUCE licensing suits the intended distribution, and choose the project's distribution licence before commercial release. No AAX target was built without an authorised SDK.
