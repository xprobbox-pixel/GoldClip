#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace aurum
{
// Explicit wet-only sample-peak guard: 32 samples lookahead, immediate linked
// gain reduction, 20 ms release. Never limits the final parallel sum. Not true peak.
class ReconstructionGuard
{
    std::array<std::array<double, 33>, 2> ring{};
    int position = 0;
    double reduction = 1, release = 0;

  public:
    static constexpr int latency = 32;
    void prepare(double rate) noexcept
    {
        ring = {};
        position = 0;
        reduction = 1;
        release = std::exp(-1 / (rate * .02));
    }
    void process(double *frame, unsigned channels, double ceiling, bool enabled) noexcept
    {
        double peak = 0, delayed[2]{};
        for (unsigned c = 0; c < channels; ++c)
        {
            ring[c][size_t(position)] = frame[c];
            delayed[c] = ring[c][size_t((position + 1) % 33)];
            for (double s : ring[c])
                peak = std::max(peak, std::abs(s));
        }
        const double target = enabled ? std::min(1., ceiling / std::max(1e-20, peak)) : 1.;
        reduction = target < reduction ? target : release * reduction + (1 - release) * target;
        for (unsigned c = 0; c < channels; ++c)
            frame[c] = delayed[c] * reduction;
        if (++position == 33)
            position = 0;
    }
};
} // namespace aurum
