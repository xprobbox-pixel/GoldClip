#pragma once
#include "Parameters.h"
namespace aurum
{
// Independent monotonic curve families, not proprietary Gold Clip equations.
class GoldProcessor
{
  public:
    static double process(double x, double ceiling, double amount, int mode, bool unity) noexcept
    {
        if (!std::isfinite(x))
            return 0;
        amount = std::clamp(finiteOr(amount, 0), 0., mode == 0 ? 2.5 : 6.);
        if (amount == 0)
            return x;
        const double c = std::max(1e-6, ceiling), u = std::abs(x) / c, a = gain(amount) - 1;
        double y = x;
        if (u < 1)
        {
            const double v = 1 - u * u;
            // Modern: short smooth polynomial; Classic: amount-dependent rational knee.
            const double shape =
                mode == 0 ? v * v * v * v * v * v : v * v * v / (1 + (2 - amount / 3) * u * u);
            y = std::copysign(c * (u + a * u * shape), x);
        }
        return unity ? y / gain(amount) : y;
    }
};
} // namespace aurum
