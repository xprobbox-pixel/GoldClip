#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace aurum
{
// BS.1770-4 K weighting and mono/stereo channel-energy sum; ungated 3 s short term.
// De Man bilinear coefficients, validated separately against FFmpeg ebur128.
class LoudnessMeter
{
    struct Filter
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<double, 2> z1{}, z2{};
        double process(double x, unsigned c) noexcept
        {
            const double y = b0 * x + z1[c];
            z1[c] = b1 * x - a1 * y + z2[c];
            z2[c] = b2 * x - a2 * y;
            return y;
        }
    } shelf, highpass;
    std::array<double, 30> bins{};
    double accumulated = 0, sum = 0;
    int binLength = 4800, count = 0, index = 0, filled = 0;

  public:
    void prepare(double rate) noexcept
    {
        shelf = {};
        highpass = {};
        bins = {};
        accumulated = sum = 0;
        count = index = filled = 0;
        binLength = std::max(1, int(std::lround(rate * .1)));
        double k = std::tan(std::acos(-1.) * 1681.974450955533 / rate), q = .7071752369554196,
               vh = std::pow(10., 3.999843853973347 / 20), vb = std::pow(vh, .4996667741545416),
               a0 = 1 + k / q + k * k;
        shelf.b0 = (vh + vb * k / q + k * k) / a0;
        shelf.b1 = 2 * (k * k - vh) / a0;
        shelf.b2 = (vh - vb * k / q + k * k) / a0;
        shelf.a1 = 2 * (k * k - 1) / a0;
        shelf.a2 = (1 - k / q + k * k) / a0;
        k = std::tan(std::acos(-1.) * 38.13547087602444 / rate);
        q = .5003270373238773;
        a0 = 1 + k / q + k * k;
        highpass.b0 = 1;
        highpass.b1 = -2;
        highpass.b2 = 1;
        highpass.a1 = 2 * (k * k - 1) / a0;
        highpass.a2 = (1 - k / q + k * k) / a0;
    }
    bool push(const double *frame, unsigned channels) noexcept
    {
        for (unsigned c = 0; c < channels; ++c)
        {
            const double y = highpass.process(shelf.process(std::clamp(frame[c], -1e6, 1e6), c), c);
            accumulated += y * y;
        }
        if (++count < binLength)
            return false;
        sum -= bins[size_t(index)];
        bins[size_t(index)] = accumulated / binLength;
        sum += bins[size_t(index)];
        index = (index + 1) % 30;
        filled = std::min(30, filled + 1);
        accumulated = 0;
        count = 0;
        return true;
    }
    void reset() noexcept
    {
        shelf.z1 = {};
        shelf.z2 = {};
        highpass.z1 = {};
        highpass.z2 = {};
        bins = {};
        accumulated = sum = 0;
        count = index = filled = 0;
    }
    double shortTerm() const noexcept
    {
        if (filled < 30)
            return std::numeric_limits<double>::quiet_NaN();
        return sum > 1e-20 ? -.691 + 10 * std::log10(std::max(0., sum) / 30)
                           : -std::numeric_limits<double>::infinity();
    }
};
} // namespace aurum
