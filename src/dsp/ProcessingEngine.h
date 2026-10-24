#pragma once
#include "ParallelMixer.h"
#include "SmoothedGain.h"
#include "Telemetry.h"
#include <limits>
namespace aurum
{
// Phase 1 implements the base-rate routing shell only; nonlinear stages are not enabled.
// Future wet chain: Box Tone -> Input Trim -> Up -> Clipper -> Alchemy -> Gold -> Down.
class ProcessingEngine
{
    SmoothedGain input, output, wet, dry, delta, bypass;
    unsigned channels = 0;
    bool ready = false;
    static double safe(double x) noexcept
    {
        return std::isfinite(x) ? x : 0;
    }

  public:
    bool prepare(double sampleRate, unsigned numChannels, Parameters parameters = {}) noexcept;
    void reset(Parameters parameters = {}) noexcept;
    int latencySamples() const noexcept
    {
        return 0;
    }
    template <class Sample>
    void process(Sample *const *audio, unsigned numChannels, int numSamples, Parameters parameters,
                 Telemetry *telemetry = nullptr, bool hostBypassed = false) noexcept
    {
        if (!ready || numChannels != channels || numSamples < 0)
            return;
        const auto p = validated(parameters);
        const auto mix = ParallelMixer::targets(p);
        input.setTarget(gain(p.inputDb));
        output.setTarget(gain(p.outputDb));
        wet.setTarget(mix.wet);
        dry.setTarget(mix.dry);
        delta.setTarget(p.delta ? 1 : 0);
        bypass.setTarget(p.bypass ? 1 : 0);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto a = input.next(), b = output.next(), w = wet.next(), d = dry.next(),
                       difference = delta.next(), pass = bypass.next();
            double original[2]{}, processed[2]{};
            for (unsigned c = 0; c < channels; ++c)
            {
                original[c] = safe(double(audio[c][i]));
                const double wetSample = safe(original[c] * a);
                const double mixed =
                    safe(ParallelMixer::combine(wetSample, original[c], {w, d}, difference) * b);
                // Immediate host bypass; plugin bypass uses a 20 ms crossfade. Both retain dry
                // unity.
                double result =
                    hostBypassed ? original[c] : safe(mixed + (original[c] - mixed) * pass);
                if (std::abs(result) > double(std::numeric_limits<Sample>::max()))
                    result = 0;
                audio[c][i] = Sample(result);
                processed[c] = double(audio[c][i]);
            }
            if (telemetry)
                telemetry->push(original, processed, channels);
        }
    }
};
} // namespace aurum
