# Phase 2A validation — AURUM CLIP 0.2.0

Build source: `f87797617103efe828c27de9d044eb3d42896728`, branch `develop/phase-2-clipping`, [PR #2](https://github.com/xprobbox-pixel/GoldClip/pull/2). This PR depends on the unmerged foundation PR #1. Main has not been merged or modified. Source report updates after the build are documentation only. Native CI [run 12](https://github.com/xprobbox-pixel/GoldClip/actions/runs/40402382259) passed both platforms. The tested merge commit `f64c315e130c11fcba42e1623a3974280172702a` has the same tree as the build source (verified locally).

## Execution status

| Check | Local macOS arm64 | Windows native CI | macOS Universal CI |
|---|---|---|---|
| Core routing/transfer tests | 1,349 checks, pass | 1,349 checks, pass | 1,349 checks, pass |
| Host/oversampling/state tests | 451 checks, pass | 451 checks, pass | 451 checks, pass |
| Actual UI render tests | 457 checks including six captures, pass | 457 checks, pass | 457 checks, pass |
| VST3 + standalone, both variants | Universal build, pass | x64 build, pass | Universal build, pass |
| Audio Units, both variants | Universal build, pass | N/A | Universal build, pass |
| pluginval 1.0.3, strictness 5 | Master + Track pass | Master + Track pass | Master + Track pass |
| Strict auval | Not run locally | N/A | Master + Track pass |
| Development bundle seals | All six strict seals pass | N/A | All six strict seals pass |

Local CTest: **2/2 suites pass**. The Universal host-test binary's x86_64 slice also passes all 451 checks when executed under Rosetta. This is translated execution on Apple Silicon, not physical Intel hardware validation. The framework-independent core suite passes all 1,349 assertions under AddressSanitizer and UndefinedBehaviorSanitizer with no reported findings; the JUCE engine itself was not sanitizer-instrumented.

## DSP measurements and tolerances

- Neutral Off output and host/plugin bypass: exact original samples after the integer compensating delay. Both precisions, mono/stereo, rates 44.1/48/88.2/96/192 kHz, blocks 1/17/257/4096. Off Delta is an exact null. Trim/mixer/mute/Delta conventions checked at float 1e-7 / double 1e-14 tolerance.
- Oversampling: **4× / 61 samples** at 44.1 and 48 kHz; **2× / 49 samples** at 88.2 and 96 kHz; **1× / 0 samples** at 192 kHz. Both sample precisions agree. Filtered impulse maxima agree with reported latency within one sample. This does not claim perfect phase equivalence at every frequency.
- Actual FIR tap counts: first stage up **119**, down **79**; second stage up **51**, down **35**. The pinned JUCE stage design and fractional-delay correction are documented in CLIPPING.md.
- Coherent 9 kHz, 48 kHz sample-rate hard-clipping test, input amplitude 2, ceiling -6 dBFS, 8192-sample DFT after warm-up: **30.5474 dB** suppression of the folded 21 kHz component relative to the same base-rate transfer curve. Minimum regression threshold 20 dB. This is one specific alias test, not a broadband rejection specification or external compliance reference.
- Hard/Classic/Modern: dense tests across ceilings -36/-18/-6/0 dBFS verify monotonicity, odd symmetry, a linear low-level region and bounded transfer-stage output. Soft knee onset/saturation slopes are continuous within 1e-7 using a 1e-5 finite-difference step.
- Harmonic regression at input sine amplitude 1.2 / ceiling 1, 8192 samples per period: Classic H3 **0.0706573**, H5 **-0.0297556**; Modern H3 **0.0726518**, H5 **-0.0171327**. Values are signed Fourier sine coefficients, not dB. Both third/fifth coefficients differ by >0.001. This establishes distinct original curves, not proprietary sonic equivalence.
- Off→Hard→Classic→Modern→Off changes on 0.01 DC: maximum measured error **1.88843e-6**, threshold 1e-5. The internal curve remains warm in Off to avoid a double-ramp gain dip.
- NaN/Inf sanitisation, finite output and restored anti-phase symmetry after invalid input pass for all three active curves. Prepared processing and mode changes produce **zero C++ new/new[] calls** in a thread-local probe. This probe does not intercept arbitrary third-party malloc calls.
- Informational local timing: 257,000 mono double samples at 48 kHz / 4× processed in about **0.037 s**, representing 5.354 s of audio. This single-machine measurement is not a real-time scheduling, stereo CPU or multi-instance budget.

## Confirmed limitations

**The nonlinear-stage ceiling is not a final peak guarantee.** In the automated driven stereo/invalid-sample transient test at -6 dBFS ceiling (C=0.501187), reconstructed maxima were about 0.5830–0.5833, approximately 1.32 dB above the stage ceiling. The test deliberately allows FIR ringing. Mix and Output Trim can raise peaks further. No hidden postmix limiter was added. A transparent wet-path reconstruction-peak policy remains Phase 2B work.

Only fixed High factors are implemented. Pristine/Extra Pristine, independent realtime/offline quality, selectable filter phase, Clip Filter LPF and broader alias/overshoot/CPU sweeps remain unimplemented. Gold, Alchemy, Box Tone, calibrated loudness, clip clock, presets/A-B and installers are later phases. The new controls work; there are no decorative selectors for unfinished DSP.

All six local Master/Track screenshots and six Windows CI screenshots at 80/100/175% were inspected; default Master/minimum Track captures from macOS CI were also checked. Essential controls/readouts are visible without overlap. The small waveform-history footer is secondary text and remains small at minimum scale. The displayed held peaks represent actual test history, including earlier routing tests, and can therefore exceed the later selected ceiling.

Not executed: manual DAW playback/automation/listening, physical Intel testing, oldest supported macOS/Windows versions, audio device loopback, live offline-export comparisons, multi-instance CPU/stress tests, full malloc instrumentation or external-reference sonic comparisons. There are no installers, Developer ID/Authenticode signatures or notarisation. Mac bundles use verified ad-hoc development seals only. These are development binaries, not a distribution-ready product.

Local logs, screenshots, source archive and downloaded CI artifacts/checksums are delivered under `release/phase-2A/`. Phase 1's separate report/artifacts remain preserved.

## Delivered native artifacts

| Platform | Built formats for both variants | Archive SHA-256 | Signing / installers |
|---|---|---|---|
| Windows x64 | VST3 + standalone | `33f48046c111c5731d3eb32f59ff67560e2eecd9fd3eb3a95ac13037612ea212` | Unsigned; no installer |
| macOS arm64/x86_64 Universal | VST3 + AU + standalone | `362c91b4e110c843858a2987bea907a26f08c83d6822e3f308a871e130f341dd` | Ad-hoc development seals; no Developer ID, notarisation or installer |

Downloaded archives were independently checked: all 19 Windows and 39 macOS manifest file hashes match; four Windows executable/module PE headers are x64; all six Mac bundles contain arm64/x86_64 and pass strict signature verification across all architectures. CI artifact IDs: Windows `12688619884`, macOS `12688459761`. Release manifest enumerates delivered file checksums. Earlier Phase 2A runs 10/11 were cancelled when newer regression fixes were published; final run 12 completed without failed checks.
