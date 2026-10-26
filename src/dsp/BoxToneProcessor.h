#pragma once
#include "SmoothedGain.h"
#include <array>
namespace aurum
{
class BoxToneProcessor
{
    static constexpr int taps = 129, delay = 64;
    std::array<std::array<double, taps>, 2> coefficients{}, history{};
    std::array<SmoothedGain, 2> blend;
    int position = 0;
    bool enabled = false;
    static double response(double f, int mode, double rate) noexcept
    {
        const auto bell = [&](double center, double width)
        {
            const double d = (f - center) / width;
            return std::exp(-.5 * d * d);
        };
        double db = mode == 0
                        ? .08 * bell(16000, 3500) - .18 * bell(22000, 3500)
                        : -.06 * bell(3000, 800) - .08 * bell(8000, 1800) - .30 * bell(20000, 3000);
        // Ultrasonic rolloff begins above 21 kHz; never fold an out-of-band target.
        if (f > 21000)
            db -= 12 * std::pow((f - 21000) / std::max(1000., rate * .5 - 21000), 4);
        return std::pow(10., db / 20);
    }

  public:
    void prepare(double rate, bool master, int mode) noexcept
    {
        enabled = master;
        position = 0;
        history = {};
        if (!master)
            return;
        for (int m = 0; m < 2; ++m)
        {
            blend[size_t(m)].prepare(rate, mode == m + 1 ? 1 : 0);
            double sum = 0;
            for (int n = 0; n < taps; ++n)
            {
                double h = 0;
                // Numerical cosine integration of the independently specified magnitude contour.
                for (int k = 0; k <= 1024; ++k)
                {
                    const double w = std::acos(-1.) * k / 1024;
                    h += (k == 0 || k == 1024 ? .5 : 1) * response(rate * k / 2048., m, rate) *
                         std::cos(w * (n - delay)) / 1024;
                }
                h *= .42 - .5 * std::cos(2 * std::acos(-1.) * n / (taps - 1)) +
                     .08 * std::cos(4 * std::acos(-1.) * n / (taps - 1));
                coefficients[size_t(m)][size_t(n)] = h;
                sum += h;
            }
            for (auto &h : coefficients[size_t(m)])
                h /= sum;
        }
    }
    int latencySamples() const noexcept
    {
        return enabled ? delay : 0;
    }
    void process(double *samples, unsigned channels, int mode) noexcept
    {
        if (!enabled)
            return;
        double weights[2];
        for (int m = 0; m < 2; ++m)
        {
            blend[size_t(m)].setTarget(mode == m + 1 ? 1 : 0);
            weights[m] = blend[size_t(m)].next();
        }
        for (unsigned c = 0; c < channels; ++c)
        {
            history[c][size_t(position)] = samples[c];
            const double flat = history[c][size_t((position - delay + taps) % taps)];
            double result = flat;
            for (int m = 0; m < 2; ++m)
                if (weights[m] != 0)
                {
                    double filtered = 0;
                    int index = position;
                    for (int n = 0; n < taps; ++n)
                    {
                        filtered += coefficients[size_t(m)][size_t(n)] * history[c][size_t(index)];
                        if (--index < 0)
                            index = taps - 1;
                    }
                    result += weights[m] * (filtered - flat);
                }
            samples[c] = result;
        }
        if (++position == taps)
            position = 0;
    }
};
} // namespace aurum
