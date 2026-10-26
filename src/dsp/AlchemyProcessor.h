#pragma once
#include "Parameters.h"
#include <array>
namespace aurum
{
// Stereo-linked instantaneous level control of a broad unity-peak bandpass.
// Parallel band subtraction preserves exact identity at amount zero.
class AlchemyProcessor
{
    double b0 = 0, a1 = 0, a2 = 0, level = 0, coefficient = 0;
    std::array<double, 2> z1{}, z2{};

  public:
    void prepare(double rate) noexcept
    {
        const double w = 2 * std::acos(-1.) * std::min(6500., rate * .35) / rate;
        const double alpha = std::sin(w) / (2 * .65), scale = 1 / (1 + alpha);
        b0 = alpha * scale;
        a1 = -2 * std::cos(w) * scale;
        a2 = (1 - alpha) * scale;
        coefficient = std::exp(-1 / (rate * .0001));
        level = 0;
        z1 = {};
        z2 = {};
    }
    void process(double *samples, unsigned channels, double ceiling, double amount) noexcept
    {
        double peak = 0;
        for (unsigned c = 0; c < channels; ++c)
            peak = std::max(peak, std::abs(samples[c]));
        const double u = std::clamp((peak / std::max(1e-6, ceiling) - .15) / .85, 0., 1.);
        level = coefficient * level + (1 - coefficient) * u * u * (3 - 2 * u);
        const double loss = std::expm1(-std::log(10.) / 20 * std::clamp(amount, 0., 2.) * level);
        for (unsigned c = 0; c < channels; ++c)
        {
            const double x = samples[c], band = b0 * x + z1[c];
            z1[c] = -a1 * band + z2[c];
            z2[c] = -b0 * x - a2 * band;
            samples[c] = x + loss * band;
        }
    }
};
} // namespace aurum
