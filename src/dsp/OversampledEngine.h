#pragma once
#include "AlchemyProcessor.h"
#include "BoxToneProcessor.h"
#include "ClipperEngine.h"
#include "GoldProcessor.h"
#include "ParallelMixer.h"
#include "ReconstructionGuard.h"
#include "SmoothedGain.h"
#include "Telemetry.h"
#include <array>
#include <juce_dsp/juce_dsp.h>
#include <limits>
#include <memory>
#include <vector>
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
    GoldProcessor goldProcessor;
    AlchemyProcessor alchemy;
    BoxToneProcessor tone;
    ReconstructionGuard guard;
    SmoothedGain goldAmount, alchemyAmount, filterAmount, unityAmount, clipActive, goldFamily,
        filterCutoff, guardCeiling;
    std::array<double, 2> lowpass{};
    double rate = 48000, osRate = 192000;
    bool isMaster = false;
    unsigned channels = 0;
    int latency = 0, position = 0, cleanPosition = 0, factor = 1;
    static double safe(double v) noexcept
    {
        return std::isfinite(v) ? v : 0;
    }

  public:
    double blockMaxClip = 0;
    int blockClipSample = 0;
    bool prepare(double sampleRate, unsigned count, Parameters parameters = {}, bool master = false)
    {
        channels = 0;
        rate = sampleRate;
        isMaster = master;
        if (!std::isfinite(rate) || rate < 8000 || rate > 384000 || (count != 1 && count != 2))
            return false;
        const auto p = validated(parameters);
        const unsigned exponent = (rate <= 48000 ? 2 : rate <= 96000 ? 1 : 0) + unsigned(p.quality);
        oversampling = std::make_unique<juce::dsp::Oversampling<Sample>>(
            count, exponent,
            p.filterPhase == 0 ? juce::dsp::Oversampling<Sample>::filterHalfBandFIREquiripple
                               : juce::dsp::Oversampling<Sample>::filterHalfBandPolyphaseIIR,
            true, true);
        oversampling->initProcessing(chunkSize);
        oversampling->reset();
        tone.prepare(rate, master, p.boxTone);
        guard.prepare(rate);
        latency = int(std::lround(oversampling->getLatencyInSamples())) + tone.latencySamples() +
                  ReconstructionGuard::latency;
        factor = int(oversampling->getOversamplingFactor());
        osRate = rate * factor;
        alchemy.prepare(osRate);
        lowpass = {};
        clipActive.prepare(osRate, p.clipMode != 0 ? 1 : 0);
        goldFamily.prepare(osRate, p.goldMode);
        filterCutoff.prepare(osRate, p.clipFilterHz);
        guardCeiling.prepare(rate, gain(p.ceilingDb));
        goldAmount.prepare(osRate, p.goldDb);
        alchemyAmount.prepare(osRate, master ? p.alchemyDb : 0);
        filterAmount.prepare(osRate, p.clipFilter ? 1 : 0);
        unityAmount.prepare(osRate, p.goldUnity ? 1 : 0);
        work.setSize(int(count), chunkSize);
        work.clear();
        for (unsigned c = 0; c < count; ++c)
        {
            originalDelay[c].assign(size_t(std::max(1, latency)), 0);
            cleanDelay[c].assign(size_t(std::max(1, latency - tone.latencySamples())), 0);
        }
        position = cleanPosition = 0;
        const auto mix = ParallelMixer::targets(p);
        input.prepare(rate, gain(p.inputDb));
        output.prepare(rate, gain(p.outputDb));
        wet.prepare(rate, mix.wet);
        dry.prepare(rate, mix.dry);
        delta.prepare(rate, p.delta ? 1 : 0);
        bypass.prepare(rate, p.bypass ? 1 : 0);
        enable.prepare(
            rate, (p.clipMode != 0 || p.goldDb > 0 || (master && p.alchemyDb > 0) || p.clipFilter)
                      ? 1
                      : 0);
        ceiling.prepare(rate * factor, gain(p.ceilingDb));
        for (int m = 0; m < 3; ++m)
            curve[size_t(m)].prepare(rate * factor,
                                     (p.clipMode == m + 1 || (p.clipMode == 0 && m == 0)) ? 1 : 0);
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
        blockMaxClip = 0;
        blockClipSample = 0;
        const auto mix = ParallelMixer::targets(p);
        input.setTarget(gain(p.inputDb));
        output.setTarget(gain(p.outputDb));
        wet.setTarget(mix.wet);
        dry.setTarget(mix.dry);
        delta.setTarget(p.delta ? 1 : 0);
        bypass.setTarget(p.bypass ? 1 : 0);
        enable.setTarget(
            (p.clipMode != 0 || p.goldDb > 0 || (isMaster && p.alchemyDb > 0) || p.clipFilter) ? 1
                                                                                               : 0);
        clipActive.setTarget(p.clipMode != 0 ? 1 : 0);
        goldFamily.setTarget(p.goldMode);
        filterCutoff.setTarget(p.clipFilterHz);
        guardCeiling.setTarget(gain(p.ceilingDb));
        goldAmount.setTarget(p.goldDb);
        alchemyAmount.setTarget(isMaster ? p.alchemyDb : 0);
        filterAmount.setTarget(p.clipFilter ? 1 : 0);
        unityAmount.setTarget(p.goldUnity ? 1 : 0);
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
                double raw[2]{}, coloured[2]{};
                for (unsigned c = 0; c < channels; ++c)
                    raw[c] = coloured[c] = safe(double(audio[c][start + i]));
                tone.process(coloured, channels, p.boxTone);
                for (unsigned c = 0; c < channels; ++c)
                {
                    const double original = raw[c];
                    double clean = safe(coloured[c] * a);
                    if (std::abs(clean) > double(std::numeric_limits<Sample>::max()))
                        clean = 0;
                    work.setSample(int(c), i, Sample(clean));
                    originalBlock[c][size_t(i)] =
                        latency == 0 ? original : originalDelay[c][size_t(position)];
                    cleanBlock[c][size_t(i)] = cleanDelay[c][size_t(cleanPosition)];
                    originalDelay[c][size_t(position)] = original;
                    cleanDelay[c][size_t(cleanPosition)] = clean;
                }
                if (++cleanPosition == int(cleanDelay[0].size()))
                    cleanPosition = 0;
                if (latency != 0 && ++position == latency)
                    position = cleanPosition = 0;
            }
            auto block = juce::dsp::AudioBlock<Sample>(work).getSubBlock(0, size_t(n));
            auto up = oversampling->processSamplesUp(block);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                const double limit = ceiling.next();
                double weights[3];
                for (int m = 0; m < 3; ++m)
                    weights[m] = curve[size_t(m)].next();
                const double clipMix = clipActive.next();
                double frame[2]{};
                for (unsigned c = 0; c < channels; ++c)
                {
                    const double x = safe(double(up.getSample(int(c), int(i))));
                    double y = 0;
                    for (int m = 0; m < 3; ++m)
                        y += weights[m] * clipSample(x, limit, ClipMode(m + 1));
                    frame[c] = x + (y - x) * clipMix;
                    if (p.clipMode != 0 && std::abs(x) > std::abs(y) && std::abs(y) > 1e-20)
                    {
                        const double reduction = 20 * std::log10(std::abs(x / y));
                        if (reduction > blockMaxClip)
                        {
                            blockMaxClip = reduction;
                            blockClipSample = start + int(i) / factor;
                        }
                    }
                }
                const double amount = alchemyAmount.next();
                if (isMaster)
                    alchemy.process(frame, channels, limit, amount);
                const double g = goldAmount.next(), unity = unityAmount.next(),
                             filtered = filterAmount.next(), family = goldFamily.next();
                const double lpf = std::exp(-2 * std::acos(-1.) *
                                            std::min(filterCutoff.next(), rate * .45) / osRate);
                for (unsigned c = 0; c < channels; ++c)
                {
                    const double modern =
                        goldProcessor.process(frame[c], limit, std::min(g, 2.5), 0, false);
                    double y =
                        (modern +
                         family * (goldProcessor.process(frame[c], limit, g, 1, false) - modern)) *
                        (1 + unity * (gain(-g) - 1));
                    lowpass[c] = (1 - lpf) * y + lpf * lowpass[c];
                    y += filtered * (lowpass[c] - y);
                    up.setSample(int(c), int(i), Sample(safe(y)));
                }
            }
            oversampling->processSamplesDown(block);
            for (int i = 0; i < n; ++i)
            {
                double guarded[2]{};
                for (unsigned c = 0; c < channels; ++c)
                    guarded[c] = safe(double(work.getSample(int(c), i)));
                guard.process(guarded, channels, guardCeiling.next(),
                              p.peakGuard && p.clipMode != 0);
                const double b = output.next(), w = wet.next(), d = dry.next(),
                             difference = delta.next(), pass = bypass.next(),
                             active = enable.next();
                double original[2]{}, processed[2]{};
                for (unsigned c = 0; c < channels; ++c)
                {
                    original[c] = originalBlock[c][size_t(i)];
                    const double clean = cleanBlock[c][size_t(i)];
                    const double clipped = guarded[c];
                    const double wetSample =
                        active == 0 ? clean : safe(clean + (clipped - clean) * active);
                    const double mixed = safe(
                        ParallelMixer::combine(wetSample, original[c], {w, d}, difference) * b);
                    double result = hostBypassed || pass == 1 ? original[c]
                                    : pass == 0               ? mixed
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
