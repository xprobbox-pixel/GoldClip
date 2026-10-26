#pragma once
#include "Parameters.h"
namespace aurum
{
// Original transfer curves: these names describe this implementation, not a model of
// any proprietary processor. Ceiling is a nonlinear-stage amplitude, not true peak.
enum class ClipMode
{
    off,
    hard,
    classic,
    modern
};
inline double clipSample(double sample, double ceiling, ClipMode mode) noexcept
{
    if (!std::isfinite(sample))
        return 0;
    if (mode == ClipMode::off)
        return sample;
    ceiling = std::clamp(finiteOr(ceiling, 1), 1e-6, 1.0);
    const double magnitude = std::abs(sample);
    if (mode == ClipMode::hard)
        return std::clamp(sample, -ceiling, ceiling);
    const double knee = mode == ClipMode::classic ? .8 : .5;
    const double span = 2 * (1 - knee);
    if (magnitude <= ceiling * knee)
        return sample;
    if (magnitude >= ceiling * (knee + span))
        return std::copysign(ceiling, sample);
    const double t = (magnitude / ceiling - knee) / span;
    const double t2 = t * t, t3 = t2 * t, t4 = t2 * t2;
    const double integral =
        mode == ClipMode::classic ? t - t3 + .5 * t4 : t - 2.5 * t4 + 3 * t4 * t - t3 * t3;
    return std::copysign(ceiling * (knee + span * integral), sample);
}
} // namespace aurum
