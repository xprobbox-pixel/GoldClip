# Phase 1 validation — 24 October 2026

Executed locally: framework-independent C++20 Release build on macOS Apple Silicon; **1,316 deterministic checks, zero failures**. Cases cover float/double, five sample rates, mono/stereo, blocks 1/17/64/257/4096, routing, parallel/Delta, bypass, gain smoothing, invalid input/parameters, callback allocation instrumentation and bounded visual telemetry. Tolerances are in `tests/DspTests.cpp`.

Native Universal JUCE compilation and host integration checks are in progress. Native CI will build and validate both variants on macOS and Windows for the Phase 1 PR; results are pending until the jobs complete. A workflow definition is not proof of a build.

Signing/notarisation, installers, manual DAW/audio-device tests, oldest-OS runtime testing and final mastering DSP are not performed in Phase 1. No clipping, Gold, Alchemy, Box Tone, oversampling, loudness accuracy or sonic-reference matching is claimed.
