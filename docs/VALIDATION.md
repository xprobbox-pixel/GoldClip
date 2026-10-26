# AURUM CLIP Phase 1 validation

Development branch: `develop/phase-1-foundation`. Pull request: https://github.com/xprobbox-pixel/GoldClip/pull/1 . The stable main branch is not merged automatically.

## Implemented scope

Shared C++20 mono/stereo float/double routing; smoothed trims; original-input dry route; linked and independent parallel mixing; wet/dry mutes; actual Delta; host/plugin bypass; validated APVTS state; real peak/overload and bounded min/max waveform telemetry; scalable original Master/Track editors; native CMake/CI.

Clipping, Gold, Alchemy, Box Tone, oversampling/latency compensation, LUFS, clip clock, trim linking, presets/A-B, final reference-style UI and installers remain later phases. There are no dummy controls for those modules. Phase 1 is not a completed mastering processor or a sonic match to the reference product.

## Executed local checks

- Release DSP suite: 1,316 checks, zero failures. Tests cover float/double, five sample rates through 192 kHz, mono/stereo, blocks 1/17/64/257/4096, routing, parallel/Delta, mutes, bit-exact settled bypass, smoothing, invalid values, allocation instrumentation and bounded telemetry.
- AddressSanitizer and UndefinedBehaviorSanitizer on the same DSP suite: 1,316 checks, zero failures, no finding emitted. This does not claim leak or thread-sanitizer coverage.
- Separate concurrent producer/consumer stress: 100,000 envelopes, 87,842 consumed and 12,158 intentional overflow drops, zero corrupt/out-of-order frames; all frames accounted for. Source and output are in `validation/`. This is not a formal race-freedom proof.
- Native Universal local compilation succeeded using the SDK's matching libc++ headers; local CTest passed 2/2. Host suite passed 16 checks, and the rendering invocation passed 22 including six real editor snapshots.

## Native CI history and corrections

- Run 7 compiled Windows successfully and passed tests/pluginval, while macOS exposed a nominal-zero APVTS rounding error. The adapter now recovers the declared 0.01-unit grid before gain conversion. Exact neutral and double-precision tests pass without weakening their tolerances. Negative-zero display formatting was also corrected.
- Run 8 (https://github.com/xprobbox-pixel/GoldClip/actions/runs/40091272933), head `fb2b00c7aec83727213a937db4691bcb9af72b3b`, passed both native platforms: 1,316 DSP checks, 16 host checks, 22 host/render checks, pluginval strictness 5 for both VST3s, and strict auval for both macOS AUs. Actual build/merge commit was `4b0fd827192941a7793622b3c0ee4db3f9470b24`.
- All run-8 artifact hashes matched (19 Windows files, 35 macOS files). Strict local resource-seal inspection found VST3 metadata added after JUCE's build-time seal; x86_64 AU/application slices were ordinarily unsigned. The final staging step now applies ad-hoc development seals after metadata and verifies all architectures. The helper passed locally on all six Universal bundles. This uses no release credentials.
- Final staging CI run: https://github.com/xprobbox-pixel/GoldClip/actions/runs/40397953385 ; source head `c6dc186cfada9d7e03166e5b173901b48209babd`. Both final native jobs passed: 1,316 DSP checks, 16 host checks, 22 host/render checks, pluginval strictness 5 for both VST3s, and strict auval for both AUs. Build/merge commit: `413b09346cc933649bedde6617eed877910ec45b`. Final downloads matched all 19 Windows and 39 macOS file hashes; archive digests match GitHub. All six downloaded macOS bundles passed local Universal architecture and strict development-seal verification.

## Unexecuted or deferred

Manual DAW listening/automation/window interaction, physical Intel Mac testing, oldest supported OS runtime testing, standalone audio-device testing, CPU profiling and nonlinear/reference-audio measurements remain unexecuted. Compilation, component tests and automated validators are separate from manual DAW testing. There are no installers in Phase 1. No Developer ID, Authenticode, release signing or notarisation is performed. Ad-hoc development integrity seals are not release signing. Review JUCE and third-party licensing before distribution.

The workstation's stale libc++ selection was isolated and corrected with a local SDK-header override; no vendor JUCE source was patched. The repository remains pinned to JUCE 9.0.4. Details are in `docs/REFERENCES.md`.

## Final artifacts

- Windows ZIP SHA-256: `9b9707c8c4f0ad4b2710fdda0451dff8a96e4923bcb03994accf7fbb30d3e91b`.
- macOS ZIP SHA-256: `d76d2add9d1f4a9053bdde186abce6523574fbaf62aa806df2e894b3f5fbe038`.
- Master and Track real JUCE snapshots inspected at 80%, 100% and 175%; essential controls/readouts stay within their panels. Secondary footer text is small at minimum scale. This is a functional foundation UI, not the final reference-style product interface.
