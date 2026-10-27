# Product architecture

Master and Track use native JUCE wrappers and the same modular C++ DSP. `ProcessingEngine` remains the framework-independent routing oracle for regression tests. The live processor uses `QualityBank<Sample>` containing six prepared `OversampledEngine<Sample>` paths (three rates × FIR/IIR), with one active path in steady state. Both float/double variants are prepared outside processBlock.

## Signal flow and latency

Original input → independent compensated dry path. Wet: Master Box Tone → Input Trim → JUCE oversampler → Clipper → Master Alchemy → Gold → optional Clip Filter → reconstruction → explicit wet sample guard → parallel mixer → final Output Trim. Flat/Off paths retain exact delayed identity at neutral settings. Host bypass immediately selects the same compensated raw input; plugin bypass has a 20 ms crossfade. Delta is aligned wet minus original dry independently of normal mixer levels/mutes.

Master's 129-tap Box Tone always reserves 64 base-rate samples; Track excludes it. The wet guard always reserves 32 samples, even when disabled. QualityBank finds the largest actual integer JUCE/filter delay across the six paths, then pads each output to the common delay and reports it to the host. This includes dry, clean Off and bypass. Audio paths never reallocate or ask the host to change PDC on quality automation. Sample-rate/layout changes reprepare and report a new delay. Inactive paths warm for 30 ms plus maximum latency before a 50 ms crossfade; only the active/new pair processes during a switch. A freshly prepared offline render selects its requested graph on its first callback. Switching a running instance to offline quality follows the same warm transition.

The FIR stages are symmetric linear phase; JUCE's optional integer correction adds fractional allpass delay. Minimum-phase IIR has frequency-dependent phase. Dry compensation aligns the declared delay and does not claim to cancel every phase difference. These differences remain observable in Delta and parallel mixes.

## DSP modules

- ClipperEngine: original odd Hard/Classic/Modern transfer functions, documented historically in CLIPPING.md; no copied proprietary equations.
- GoldProcessor: let `u=abs(x)/C`, `a=10^(G/20)-1`, `v=1-u²`. For u<1, return `sign(x)*C*(u+a*u*shape)`. Modern shape is `v^6`, G≤2.5 dB. Classic is `v³/(1+(2-G/3)*u²)`, G≤6 dB. Outside the ceiling, return x. The families are odd, peak-anchored, monotonic over validated ranges and smooth at zero/ceiling; Classic curvature changes with amount. Optional Unity independently multiplies by 10^(-G/20). Both families are smoothed/crossfaded at the oversampled rate. These are independent approximations, not calibrated reference equations.
- AlchemyProcessor: stereo-linked peak proximity to C drives a smoothstep with 0.1 ms control smoothing. A unity-peak second-order bandpass near 6.5 kHz/Q0.65 feeds parallel subtraction scaled by up to -2 dB. Low-frequency events affect the linked controller, so coincident high-frequency material is modulated. Low/high band response and dynamic results are measured. Amount zero is identity; this is not a static shelf or slow compressor.
- BoxToneProcessor: independently specified subtle Gaussian magnitude contours, ultrasonic taper above 21 kHz, numerical cosine integration, Blackman window, DC normalisation; two 129-tap symmetric FIRs. Flat returns delayed unfiltered input. Mode weights ramp over 20 ms. Track does no tone FIR work.
- ReconstructionGuard: explicit wet-only sample-peak gain control. A 33-cell ring observes the current and 32 lookahead samples, linked across stereo. Needed gain reduces immediately and releases over 20 ms. Disabled guard releases to unity; output remains delayed. This is a disclosed safeguard, not Gold processing, not an inter-sample true-peak limiter and not a limiter on the final mix.
- LoudnessMeter: BS.1770-4 K-weighting, De Man bilinear shelf/high-pass coefficients, mono/stereo energy weighting, thirty 100 ms bins. No fabricated startup value, no integrated/gated loudness claim. Valid silence is minus infinity after a full 3 s. Exported signals are independently checked against FFmpeg ebur128.

Clip Filter is an optional one-pole low-pass inside the oversampled chain. Its frequency setting is capped below host Nyquist. No extra hidden normalisation, final-mix limiter, saturation or automatic input/output gain is present.

## Real-time boundaries

ProcessBlock reads cached atomic APVTS scalars. Working buffers, six filter graphs, delay rings and histories are prepared outside the callback. Arbitrary host blocks split into at most 256 samples; no callback string construction, allocation, file access, mutex or UI calls. Gains/amounts ramp 20 ms. Invalid samples/intermediate arithmetic become zero; denormals are disabled by the JUCE callback guard. C++ allocation probes cover processing, mode and quality changes; the probe does not assert complete interception of arbitrary malloc calls.

Telemetry uses fixed SPSC 1024-envelope storage with acquire/release cursors, 64 audio samples per envelope and min/max stereo aggregation. Overflow drops only visual frames. Sample peaks, LUFS and maximum stage attenuation run with the editor closed. The UI drains at most 1024 envelopes at 30 Hz and draws the latest 400. Waveform Off stops capture but leaves analysis running. Clip amount/time uses atomic publication with a bounded sequence check; a racing timestamp is temporarily unavailable rather than fabricated. Timeline values require playing host transport; no wall-clock substitute is made. Pipeline delay means timestamps are approximate processing positions, particularly under minimum phase.

## UI and state

Both vector editors preserve the host's HiDPI transform and scale their own child canvas 80–175%. Master has the full landscape meter/waveform/mixer layout; Track is compact, with independent mixing available in Settings. All visible controls act on real parameters, actual analysis or real preset/preferences operations.

APVTS schema 3 keeps existing IDs and appends Gold/quality/master DSP/preferences. Modern/Classic Gold have separate correctly ranged amounts. Master-only IDs are absent in Track. State validates finite complete transactions and rejects incompatible variants/future schemas. Legacy schemas migrate new effects to defaults. PresetManager serialises both A/B memories, active/tracking state and parameters; user saves use temporary files and atomic replacement outside processing. Factory states are deterministic. Host size takes precedence over global UI preferences. User presets/preferences remain local and survive uninstall.

Native installer and validation workflows are separate from optional credentialled signing. Native CI performs actual plugin validation, measurements, selectable installer builds and ephemeral install/payload/uninstall checks. Signing/notarisation cannot be inferred from successful compilation.
