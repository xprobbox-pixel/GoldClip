# Independent implementation references

Reviewed 24 October 2026:

- https://www.schwabedigital.com/goldclip
- https://www.schwabedigital.com/s/GoldClip.pdf
- https://www.schwabedigital.com/sdu/gold
- https://www.schwabedigital.com/sdu/alchemy
- https://www.schwabedigital.com/sdu/clippers
- https://www.schwabedigital.com/sdu/oversampling

The fetched manual identifies itself as Operations Manual 2025.2, dated 1 August 2025. The supplied development brief calls its reference v1.5 announced June 2026; that version/date was not independently established from the fetched manual. Do not describe the older manual as verified v1.5 documentation.

Public documentation guides future observable behavior and processing order. It does not reveal proprietary equations. This repository contains no extracted reference code, logos, screenshots or graphical assets. AURUM CLIP is independent and unaffiliated with Schwabe Digital. No sonic identity or measured match is claimed. A future calibration harness should accept legitimately obtained reference renders without committing licensed audio to this public repository.

JUCE 9.0.4 is pinned to commit 3fb5cbc0340758596dac39f7c8de47d562b4896e, verified against the upstream release tag. The official release is https://github.com/juce-framework/JUCE/releases/tag/9.0.4 . Local checkout configuration enforces the same Git HEAD; tracked dependency modifications are not certified by this check.

Toolchain investigation: the local CLT selected libc++ version 10000 headers despite Clang 17 and the macOS 15.5 SDK. This caused `shared_ptr` deduction and availability errors in JUCE 9 (and also the maintained JUCE 8 security release). Explicitly using the selected SDK's libc++ headers (version 190102) resolved the failing JUCE 9 CoreMIDI syntax check. Final builds retain the JUCE 9.0.4 pin with no vendor patches. The local full build uses `-nostdinc++ -isystem /Library/Developer/CommandLineTools/SDKs/MacOSX15.5.sdk/usr/include/c++/v1` as CMAKE_CXX_FLAGS; native CI uses its ordinary matched toolchain. This is an environment-specific workaround, not a general requirement to downgrade JUCE.
