# Phase 2A architecture

`aurum_dsp` is a framework-independent C++20 library. Master and Track have separate JUCE wrappers/identities, shared engine code and the same working routing and clipping controls. No Track Alchemy/Box Tone controls exist. Master-only DSP begins in a later phase.

## Implemented routing

Original input splits into dry and wet. Wet uses smoothed Input Trim followed by the oversampled clipper. Linked mixing uses `wet = blend`, `dry = 1 - blend`; independent mixing uses dB-domain Wet/Dry levels and separate mutes. Delta substitutes wet minus original dry, ignoring normal mixer gains and mutes. Smoothed Output Trim affects the final combined output, including Delta. Plugin bypass crossfades to original dry over 20 ms; the host bypass callback immediately provides delayed unity dry. Gain targets ramp linearly in amplitude over 20 ms. Mono remains mono; stereo channels use the same gains and never crossfeed. Both float and double processing retain native output precision. There is no internal limiter. Independent sums can exceed full scale.

The plugin uses `OversampledEngine<Sample>` with JUCE's maximum-quality equiripple FIR oversampler and integer latency compensation. Original and unfiltered trimmed paths use prepared ring buffers matching the reported latency. Off selects the clean trimmed delay exactly, avoiding FIR coloration; its internal oversampled path continues for smooth engagement. The original `ProcessingEngine` remains a framework-independent routing oracle and regression suite.

The engine chunks arbitrary host blocks into at most 256 base-rate samples without allocation. Per-channel buffers and delays are allocated only during prepare. Input and mixer controls ramp at base rate; the stage ceiling and three curve weights ramp at the oversampled rate. Mode engagement crossfades clean and filtered wet paths over 20 ms. Reporting latency does not change when Off/Hard/Classic/Modern is automated. Sample-rate changes reprepare both precisions and report their new integer delay. Finite path gains/sums remain capable of exceeding full scale; this is not a limiter. [Clipping design](CLIPPING.md) describes reconstruction overshoot and phase limits.

## Future wet-chain contract

Box Tone → Input Trim → Upsampling → Clipper → Alchemy → Gold → Downsampling → mixer → final Output Trim.

This is the architecture contract, not a claim these stages currently exist. Phase 2A implements fixed-quality oversampling and clipping; Phase 2B broadens quality/filter selection; Phase 3 adds distinct monotonic Gold curves; Phase 4 adds Alchemy/Box Tone; later phases complete presets/A-B, calibrated loudness/clip clock, interface and installers. Processing stage interfaces should be introduced alongside their implementations and tests, rather than empty no-op engines.

## Thread boundaries

Audio: one scalar atomic parameter snapshot per block; no property-tree operations, strings, locks, file access, allocation or UI calls. The base-rate oracle uses fixed-size stack data; the oversampled engine uses prepared fixed-capacity working buffers and rings. Nonfinite input samples and nonfinite intermediate arithmetic become zero; finite summed audio is never silently limited. Denormal mode is controlled by the JUCE callback guard.

Audio → editor: fixed 1024-element single-producer/single-consumer envelope queue, 64 host samples per envelope. Acquire/release cursors publish complete frames. Stereo aggregation takes min/max across both channels, preserving anti-phase peaks. Queue overflow drops visual frames and increments a counter. Atomic sample peaks and overload events continue with the editor closed. There is one consumer editor. UI drains at most 1024 frames per 30 Hz tick. The displayed 400-envelope history covers 25,600 samples (~0.53 s at 48 kHz), not a fixed seconds timebase.

GUI: custom vector controls, black/gold original branding, a transformed controls layer at 80–175%, fixed variant aspect ratio; host editor HiDPI transform is preserved. Actual input envelopes are grey and actual output envelopes gold. Blue ceiling guides show the selected nonlinear-stage amplitude; reconstructed/output-trimmed waveform peaks may exceed them. Gold waveform semantics are deferred. No LUFS text appears because loudness analysis is not implemented.

Effective DSP values are rounded to the declared 0.01-unit parameter grid at the block boundary; this removes platform-dependent floating-point offsets at nominal 0 dB and 100% blend without weakening sample-precision tests.

State: stable version-1 parameter IDs, complete validated APVTS transactions, variant guard and schema guard. Schema 2 appends clipMode and ceiling IDs. Schema 1 sessions migrate with clipMode Off and ceiling 0 dBFS, even when restored over an engaged clipper. Unknown future schemas, malformed/missing/duplicated known parameters and nonfinite values reject the transaction. Finite numeric values clamp to declared ranges. Width is bounded to variant scaling limits. State parsing and serialization are host lifecycle operations, outside the callback. UI size currently persists in host state, not a global preferences file.
