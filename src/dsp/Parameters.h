#pragma once
#include <algorithm>
#include <cmath>
namespace aurum
{
struct Parameters
{
    double inputDb = 0, outputDb = 0, blend = 1, wetDb = 0, dryDb = -60;
    bool linked = true, wetMute = false, dryMute = false, delta = false, bypass = false;
    double ceilingDb = 0;
    int clipMode = 0;
    double goldDb = 0, alchemyDb = 0, clipFilterHz = 20000, trimOffset = 0;
    int goldMode = 0, boxTone = 0, quality = 0, offlineQuality = 0, filterPhase = 0, meterScale = 0;
    bool goldUnity = false, clipFilter = false, trimLinked = false, waveform = true,
         peakGuard = true;
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
    p.ceilingDb = std::clamp(finiteOr(p.ceilingDb, 0), -36.0, 0.0);
    p.clipMode = std::clamp(p.clipMode, 0, 3);
    p.goldMode = std::clamp(p.goldMode, 0, 1);
    p.goldDb = std::clamp(finiteOr(p.goldDb, 0), 0., p.goldMode == 0 ? 2.5 : 6.);
    p.alchemyDb = std::clamp(finiteOr(p.alchemyDb, 0), 0., 2.);
    p.boxTone = std::clamp(p.boxTone, 0, 2);
    p.quality = std::clamp(p.quality, 0, 2);
    p.offlineQuality = std::clamp(p.offlineQuality, 0, 2);
    p.meterScale = std::clamp(p.meterScale, 0, 2);
    p.filterPhase = std::clamp(p.filterPhase, 0, 1);
    p.clipFilterHz = std::clamp(finiteOr(p.clipFilterHz, 20000), 4000., 22000.);
    p.trimOffset = std::clamp(finiteOr(p.trimOffset, 0), -24., 24.);
    return p;
}
} // namespace aurum
