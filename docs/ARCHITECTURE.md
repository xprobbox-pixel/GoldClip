# Phase 1 architecture

`aurum_dsp` is a framework-independent C++20 library. Master and Track have separate JUCE wrappers/identities, shared engine code and the same working Phase 1 controls. No Track Alchemy/Box Tone controls exist. Master-only DSP begins in a later phase.

## Implemented routing

Original input splits into dry and wet. Wet uses smoothed Input Trim. Linked mixing uses `wet = blend`, `dry = 1 - blend`; independent mixing uses dB-domain Wet/Dry levels and separate mutes. Delta substitutes wet minus original dry, ignoring normal mixer gains and mutes. Smoothed Output Trim affects the final combined output, including Delta. Plugin bypass crossfades to original dry over 20 ms; the host bypass callback immediately provides unity dry. Gain targets ramp linearly in amplitude over 20 ms. Mono remains mono; stereo channels use the same gains and never crossfeed. Both float and double processing retain native output precision. There is no internal limiter. Independent sums can exceed full scale.

The host reports zero latency because Phase 1 contains no delayed/filtering stages. Dry and wet are therefore exactly sample aligned. No oversampling/filter selector is exposed. Future latency must come from the actual oversampling/filter chain, including a preallocated compensating dry delay and host bypass alignment. Host block size does not determine internal buffer capacity: the current engine is frame-based and handles arbitrary nonnegative lengths without scratch allocation.

## Future wet-chain contract

Box Tone → Input Trim → Upsampling → Clipper → Alchemy → Gold → Downsampling → mixer → final Output Trim.

This is the architecture contract, not a claim these stages currently exist. Phase 2 introduces oversampling and independently validated clipping; Phase 3 adds distinct monotonic Gold curves; Phase 4 adds Alchemy/Box Tone; later phases complete presets/A-B, calibrated loudness/clip clock, interface and installers. Processing stage interfaces should be introduced alongside their implementations and tests, rather than empty no-op engines.

## Thread boundaries

Audio: one scalar atomic parameter snapshot per block; no property-tree operations, strings, locks, file access, allocation or UI calls. The core uses fixed-size stack data. Nonfinite input samples and nonfinite intermediate arithmetic become zero; finite summed audio is never silently limited. Denormal mode is controlled by the JUCE callback guard.

Audio → editor: fixed 1024-element single-producer/single-consumer envelope queue, 64 host samples per envelope. Acquire/release cursors publish complete frames. Stereo aggregation takes min/max across both channels, preserving anti-phase peaks. Queue overflow drops visual frames and increments a counter. Atomic sample peaks and overload events continue with the editor closed. There is one consumer editor. UI drains at most 1024 frames per 30 Hz tick. The displayed 400-envelope history covers 25,600 samples (~0.53 s at 48 kHz), not a fixed seconds timebase.

GUI: custom vector controls, black/gold original branding, a transformed controls layer at 80–175%, fixed variant aspect ratio; host editor HiDPI transform is preserved. Actual input envelopes are grey and actual output envelopes gold. This foundation does not implement the final Gold/clip semantics or a clipping ceiling line. No LUFS text appears because loudness analysis is not implemented.

Effective DSP values are rounded to the declared 0.01-unit parameter grid at the block boundary; this removes platform-dependent floating-point offsets at nominal 0 dB and 100% blend without weakening sample-precision tests.

State: stable version-1 parameter IDs, complete validated APVTS transactions, variant guard and schema guard. Unknown future schemas, malformed/missing/duplicated known parameters and nonfinite values reject the transaction. Finite numeric values clamp to declared ranges. Width is bounded to variant scaling limits. State parsing and serialization are host lifecycle operations, outside the callback. UI size currently persists in host state, not a global preferences file.
