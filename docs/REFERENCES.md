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

JUCE 8.0.16-security is pinned to commit ea77444750de45e036e0279bb0291144bf8e3688, verified against the upstream release tag. The official release is https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security . Local checkout configuration enforces the same Git HEAD; tracked dependency modifications are not certified by this check.

Compatibility decision: JUCE 9.0.4 was attempted first. Its CoreMIDI code failed against the installed Apple Clang 17 / libc++ headers (`std::shared_ptr` deduction from `unique_ptr`). The maintained 8.0.16-security release was selected rather than patching vendor code. Native CI and local checks must validate this final pin. The security release is documented at https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security .
