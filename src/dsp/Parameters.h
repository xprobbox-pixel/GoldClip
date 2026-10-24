#pragma once
#include <algorithm>
#include <cmath>
namespace aurum
{
struct Parameters
{
    double inputDb = 0, outputDb = 0, blend = 1, wetDb = 0, dryDb = -60;
    bool linked = true, wetMute = false, dryMute = false, delta = false, bypass = false;
};
inline double gain(double db) noexcept
{
    return std::pow(10.0, db / 20.0);
}
inline double finiteOr(double value, double fallback) noexcept
{
    return std::isfinite(value) ? value : fallback;
}
inline Parameters validated(Parameters p) noexcept
{
    p.inputDb = std::clamp(finiteOr(p.inputDb, 0), -12.0, 12.0);
    p.outputDb = std::clamp(finiteOr(p.outputDb, 0), -12.0, 12.0);
    p.blend = std::clamp(finiteOr(p.blend, 1), 0.0, 1.0);
    p.wetDb = std::clamp(finiteOr(p.wetDb, 0), -60.0, 6.0);
    p.dryDb = std::clamp(finiteOr(p.dryDb, -60), -60.0, 6.0);
    return p;
}
} // namespace aurum
