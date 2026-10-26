# Phase 2A clipping contract (historical)

The transfer curves remain in use. Current selectable quality, Gold/Alchemy routing and explicit wet guard are documented in ARCHITECTURE.md and the release report. The fixed-quality/no-guard boundaries below describe Phase 2A artifacts, not the final product.

These are original symmetric memoryless transfer functions. Hard, Classic and Modern do not claim to reproduce Schwabe Digital's proprietary implementation or its sound. Gold, Alchemy and Box Tone are not part of this phase.

Let `C=10^(ceilingDb/20)`, `u=abs(x)/C`. Off returns the trimmed clean path, with compensating delay and no FIR coloration. Hard returns `sign(x)*min(abs(x),C)`.

For Classic use knee `k=0.8`; Modern uses `k=0.5`. Below k, both return x. Above `2-k`, both return `sign(x)*C`. Between those bounds set `d=2*(1-k)`, `t=(u-k)/d`:

- Classic magnitude: `C*(k+d*(t-t^3+0.5*t^4))`.
- Modern magnitude: `C*(k+d*(t-2.5*t^4+3*t^5-t^6))`.

The derivatives in the knee are respectively `1-3t²+2t³` and `1-10t³+15t⁴-6t⁵`: nonnegative, joining slope 1 to slope 0. The curves are odd, monotonic and bounded inside the nonlinear stage. Ceiling accepts -36 to 0 dBFS. Nonfinite samples become silence. Audio parameters use 20 ms amplitude-linear smoothing; changing curves crossfades their instantaneous outputs and changing Off crossfades the aligned clean/filtered paths. Transitions are not asserted to maintain the instantaneous ceiling.

## Oversampling

JUCE 9.0.4 `dsp::Oversampling`, maximum-quality equiripple half-band FIR, integer latency enabled. Two stages (4×) for rates <=48 kHz, one (2×) <=96 kHz, a dummy stage (1×) above that. The current supported test matrix is 44.1/48/88.2/96/192 kHz, mono/stereo, float/double, blocks 1/17/257/4096. There is no realtime/offline quality selector yet and offline rendering follows exactly the same settings.

Pinned JUCE stage design parameters, normalised transition width and requested stopband design attenuation:

| Stage | Up width / design attenuation | Down width / design attenuation |
|---|---|---|
| First 2× | 0.05 / -90 dB | 0.06 / -75 dB |
| Second 2× | 0.10 / -80 dB | 0.12 / -65 dB |

These are filter design inputs, **not measured system alias-rejection claims**. Actual coefficient lengths, reported delay and a specific alias measurement are printed by the automated host tests and recorded in [Phase 2A validation](PHASE2-VALIDATION.md). JUCE's integer latency correction adds a fractional-delay allpass when necessary. FIR stages themselves are linear phase; the final combined phase response must not be described as perfectly linear phase across the entire spectrum. Aligned dry compensation matches the reported delay, not every frequency-dependent filter response.

All storage is prepared outside the callback. Audio processing uses internal chunks <=256 host samples and bounded arrays/rings. No property-tree access or locks occur in the callback. The engine keeps processing with the editor closed.

## Ceiling and reconstruction boundary

The nonlinear transfer is bounded by C. Reconstructed base-rate output can ring above C, especially around transients. Parallel dry/wet addition and Output Trim can raise it further. No postfilter clipping, saturation, automatic gain or true-peak limiter has been added to conceal this. The control is named **Clip Ceiling**, not True-Peak Limit. The overload warning observes actual output sample peaks.

The test suite measures a 9 kHz sine at 48 kHz, amplitude 2, hard ceiling -6 dBFS. A coherent 8192-sample DFT after warm-up compares the folded third harmonic at 21 kHz against the same hard clipper at 1×. This is one explicit regression signal, not a broadband alias guarantee or a trusted external compliance reference. Broader tone sweeps, CPU budgets, quality modes, LPF, filter options and anti-overshoot policy belong to Phase 2B. Manual listening and multi-DAW testing remain required before release.
