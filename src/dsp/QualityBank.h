#pragma once
#include "OversampledEngine.h"
namespace aurum
{
// All filter graphs and compensation storage are prepared outside the callback.
// Only the active path runs normally; a new path warms, then crossfades. Fixed max
// PDC avoids host latency changes during quality/filter automation or offline mode.
template <class Sample> class QualityBank
{
    static constexpr int chunk = 256;
    std::array<OversampledEngine<Sample>, 6> engines;
    std::array<juce::AudioBuffer<Sample>, 6> buffers;
    std::array<std::array<std::vector<Sample>, 2>, 6> compensation;
    std::array<int, 6> positions{};
    std::array<std::vector<double>, 2> originalDelay;
    int originalPosition = 0, latency = 0, active = 0, next = -1, warm = 0, fadePosition = 0,
        fadeLength = 2400;
    bool hasProcessed = false;
    unsigned channels = 0;
    double rate = 48000;
    void run(int index, int length, Parameters p, bool bypassed) noexcept
    {
        p.quality = index % 3;
        p.filterPhase = index / 3;
        engines[size_t(index)].process(buffers[size_t(index)].getArrayOfWritePointers(), channels,
                                       length, p, nullptr, bypassed);
        auto &delay = compensation[size_t(index)];
        const int size = int(delay[0].size());
        if (size == 0)
            return;
        int &position = positions[size_t(index)];
        for (int i = 0; i < length; ++i)
        {
            for (unsigned c = 0; c < channels; ++c)
            {
                auto &s = buffers[size_t(index)].getWritePointer(int(c))[i];
                auto &cell = delay[c][size_t(position)];
                std::swap(s, cell);
            }
            if (++position == size)
                position = 0;
        }
    }

  public:
    bool prepare(double sampleRate, unsigned count, Parameters p = {}, bool master = false)
    {
        rate = sampleRate;
        channels = count;
        latency = 0;
        originalPosition = 0;
        positions = {};
        next = -1;
        if (!std::isfinite(rate) || rate < 8000 || rate > 384000 || (count != 1 && count != 2))
        {
            channels = 0;
            return false;
        }
        p = validated(p);
        hasProcessed = false;
        if (p.trimLinked)
            p.outputDb = std::clamp(p.trimOffset - p.inputDb, -12., 12.);
        active = p.quality + 3 * p.filterPhase;
        for (int key = 0; key < 6; ++key)
        {
            auto initial = p;
            initial.quality = key % 3;
            initial.filterPhase = key / 3;
            if (!engines[size_t(key)].prepare(rate, count, initial, master))
                return false;
            latency = std::max(latency, engines[size_t(key)].latencySamples());
            buffers[size_t(key)].setSize(int(count), chunk);
        }
        for (int key = 0; key < 6; ++key)
            for (unsigned c = 0; c < count; ++c)
                compensation[size_t(key)][c].assign(
                    size_t(latency - engines[size_t(key)].latencySamples()), Sample{});
        for (unsigned c = 0; c < count; ++c)
            originalDelay[c].assign(size_t(std::max(1, latency)), 0);
        fadeLength = std::max(1, int(rate * .05));
        return true;
    }
    int latencySamples() const noexcept
    {
        return latency;
    }
    int oversamplingFactor() const noexcept
    {
        return engines[size_t(active)].oversamplingFactor();
    }
    void process(Sample *const *audio, unsigned count, int length, Parameters parameters,
                 Telemetry *telemetry = nullptr, bool bypassed = false,
                 double timeline = std::numeric_limits<double>::quiet_NaN()) noexcept
    {
        if (count != channels || channels == 0 || length <= 0)
            return;
        auto p = validated(parameters);
        if (p.trimLinked)
            p.outputDb = std::clamp(p.trimOffset - p.inputDb, -12., 12.);
        const int requested = p.quality + 3 * p.filterPhase;
        if (!hasProcessed)
        {
            active = requested;
            hasProcessed = true;
        }
        if (next < 0 && requested != active)
        {
            next = requested;
            warm = latency + int(rate * .03);
            fadePosition = 0;
        }
        for (int offset = 0; offset < length; offset += chunk)
        {
            const int n = std::min(chunk, length - offset);
            std::array<std::array<double, chunk>, 2> raw{};
            for (unsigned c = 0; c < channels; ++c)
                for (int i = 0; i < n; ++i)
                {
                    const Sample value =
                        std::isfinite(audio[c][offset + i]) ? audio[c][offset + i] : Sample{};
                    raw[c][size_t(i)] = double(value);
                    buffers[size_t(active)].setSample(int(c), i, value);
                    if (next >= 0)
                        buffers[size_t(next)].setSample(int(c), i, value);
                }
            auto processing = p;
            run(active, n, processing, bypassed);
            if (next >= 0)
                run(next, n, processing, bypassed);
            if (telemetry && !bypassed && !p.bypass && engines[size_t(active)].blockMaxClip > 0)
                telemetry->recordClip(
                    float(engines[size_t(active)].blockMaxClip),
                    timeline + (offset + engines[size_t(active)].blockClipSample) / rate);
            for (int i = 0; i < n; ++i)
            {
                double blend = 0;
                if (next >= 0)
                {
                    if (warm > 0)
                        --warm;
                    else
                        blend = std::min(1., double(++fadePosition) / fadeLength);
                }
                double original[2]{}, result[2]{};
                for (unsigned c = 0; c < channels; ++c)
                {
                    auto &delayed = originalDelay[c][size_t(originalPosition)];
                    original[c] = delayed;
                    delayed = raw[c][size_t(i)];
                    double y = double(buffers[size_t(active)].getSample(int(c), i));
                    if (next >= 0)
                        y += (double(buffers[size_t(next)].getSample(int(c), i)) - y) * blend;
                    audio[c][offset + i] = Sample(std::isfinite(y) ? y : 0);
                    result[c] = double(audio[c][offset + i]);
                }
                if (++originalPosition == latency)
                    originalPosition = 0;
                if (telemetry)
                    telemetry->push(original, result, channels);
            }
            if (next >= 0 && fadePosition >= fadeLength)
            {
                active = next;
                next = -1;
            }
        }
    }
};
} // namespace aurum
