#pragma once
#include "ClipperEngine.h"
#include "ParallelMixer.h"
#include "SmoothedGain.h"
#include "Telemetry.h"
#include <juce_dsp/juce_dsp.h>
namespace aurum
{
// Fixed-quality FIR analysis/processing, prepared off the audio thread. All modes
// share integer latency, including Off and host bypass. No callback allocations.
template <class Sample> class OversampledEngine
{
    static constexpr int chunkSize = 256;
    std::unique_ptr<juce::dsp::Oversampling<Sample>> oversampling;
    juce::AudioBuffer<Sample> work;
    std::array<std::vector<double>, 2> originalDelay, cleanDelay;
    std::array<std::array<double, chunkSize>, 2> originalBlock{}, cleanBlock{};
    std::array<SmoothedGain, 3> curve;
    SmoothedGain input, output, wet, dry, delta, bypass, enable, ceiling;
    unsigned channels = 0;
    int latency = 0, position = 0, factor = 1;
    static double safe(double v) noexcept
    {
        return std::isfinite(v) ? v : 0;
    }

  public:
    bool prepare(double rate, unsigned count, Parameters parameters = {})
    {
        channels = 0;
        if (!std::isfinite(rate) || rate < 8000 || rate > 384000 || (count != 1 && count != 2))
            return false;
        const auto p = validated(parameters);
        const unsigned exponent = rate <= 48000 ? 2 : rate <= 96000 ? 1 : 0;
        oversampling = std::make_unique<juce::dsp::Oversampling<Sample>>(
            count, exponent, juce::dsp::Oversampling<Sample>::filterHalfBandFIREquiripple, true,
            true);
        oversampling->initProcessing(chunkSize);
        oversampling->reset();
        latency = int(std::lround(oversampling->getLatencyInSamples()));
        factor = int(oversampling->getOversamplingFactor());
        work.setSize(int(count), chunkSize);
        work.clear();
        for (unsigned c = 0; c < count; ++c)
        {
            originalDelay[c].assign(size_t(std::max(1, latency)), 0);
            cleanDelay[c].assign(size_t(std::max(1, latency)), 0);
        }
        position = 0;
        const auto mix = ParallelMixer::targets(p);
        input.prepare(rate, gain(p.inputDb));
        output.prepare(rate, gain(p.outputDb));
        wet.prepare(rate, mix.wet);
        dry.prepare(rate, mix.dry);
        delta.prepare(rate, p.delta ? 1 : 0);
        bypass.prepare(rate, p.bypass ? 1 : 0);
        enable.prepare(rate, p.clipMode != 0 ? 1 : 0);
        ceiling.prepare(rate * factor, gain(p.ceilingDb));
        for (int m = 0; m < 3; ++m)
            curve[size_t(m)].prepare(rate * factor, p.clipMode == m + 1 ? 1 : 0);
        channels = count;
        return true;
    }
    int latencySamples() const noexcept
    {
        return latency;
    }
    int oversamplingFactor() const noexcept
    {
        return factor;
    }
    void process(Sample *const *audio, unsigned count, int length, Parameters parameters,
                 Telemetry *telemetry = nullptr, bool hostBypassed = false) noexcept
    {
        if (count != channels || channels == 0 || length <= 0)
            return;
        const auto p = validated(parameters);
        const auto mix = ParallelMixer::targets(p);
        input.setTarget(gain(p.inputDb));
        output.setTarget(gain(p.outputDb));
        wet.setTarget(mix.wet);
        dry.setTarget(mix.dry);
        delta.setTarget(p.delta ? 1 : 0);
        bypass.setTarget(p.bypass ? 1 : 0);
        enable.setTarget(p.clipMode != 0 ? 1 : 0);
        ceiling.setTarget(gain(p.ceilingDb));
        if (p.clipMode != 0)
            for (int m = 0; m < 3; ++m)
                curve[size_t(m)].setTarget(p.clipMode == m + 1 ? 1 : 0);
        for (int start = 0; start < length; start += chunkSize)
        {
            const int n = std::min(chunkSize, length - start);
            for (int i = 0; i < n; ++i)
            {
                const double a = input.next();
                for (unsigned c = 0; c < channels; ++c)
                {
                    const double original = safe(double(audio[c][start + i]));
                    double clean = safe(original * a);
                    // Prevent narrowing overflow before the FIR; invalid material is silence.
                    if (std::abs(clean) > double(std::numeric_limits<Sample>::max()))
                        clean = 0;
                    work.setSample(int(c), i, Sample(clean));
                    originalBlock[c][size_t(i)] =
                        latency == 0 ? original : originalDelay[c][size_t(position)];
                    cleanBlock[c][size_t(i)] =
                        latency == 0 ? clean : cleanDelay[c][size_t(position)];
                    originalDelay[c][size_t(position)] = original;
                    cleanDelay[c][size_t(position)] = clean;
                }
                if (latency != 0 && ++position == latency)
                    position = 0;
            }
            auto block = juce::dsp::AudioBlock<Sample>(work).getSubBlock(0, size_t(n));
            auto up = oversampling->processSamplesUp(block);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                const double limit = ceiling.next();
                double weights[3];
                for (int m = 0; m < 3; ++m)
                    weights[m] = curve[size_t(m)].next();
                for (unsigned c = 0; c < channels; ++c)
                {
                    const double x = double(up.getSample(int(c), int(i)));
                    double y = 0;
                    for (int m = 0; m < 3; ++m)
                        y += weights[m] * clipSample(x, limit, ClipMode(m + 1));
                    up.setSample(int(c), int(i), Sample(safe(y)));
                }
            }
            oversampling->processSamplesDown(block);
            for (int i = 0; i < n; ++i)
            {
                const double b = output.next(), w = wet.next(), d = dry.next(),
                             difference = delta.next(), pass = bypass.next(),
                             active = enable.next();
                double original[2]{}, processed[2]{};
                for (unsigned c = 0; c < channels; ++c)
                {
                    original[c] = originalBlock[c][size_t(i)];
                    const double clean = cleanBlock[c][size_t(i)];
                    const double clipped = safe(double(work.getSample(int(c), i)));
                    const double wetSample =
                        active == 0 ? clean : safe(clean + (clipped - clean) * active);
                    const double mixed = safe(
                        ParallelMixer::combine(wetSample, original[c], {w, d}, difference) * b);
                    double result = hostBypassed || pass == 1
                                        ? original[c]
                                        : safe(mixed + (original[c] - mixed) * pass);
                    if (std::abs(result) > double(std::numeric_limits<Sample>::max()))
                        result = 0;
                    audio[c][start + i] = Sample(result);
                    processed[c] = double(audio[c][start + i]);
                }
                if (telemetry)
                    telemetry->push(original, processed, channels);
            }
        }
    }
};
} // namespace aurum
