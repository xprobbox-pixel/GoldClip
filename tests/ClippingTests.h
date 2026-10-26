#pragma once
#include "dsp/OversampledEngine.h"
#include <chrono>
#include <iostream>
#include <vector>
namespace clipping_tests
{
template <class T, class Check> void engineTests(Check check)
{
    for (double rate : {44100., 48000., 88200., 96000., 192000.})
        for (unsigned channels : {1u, 2u})
            for (int block : {1, 17, 257, 4096})
            {
                aurum::OversampledEngine<T> engine;
                check(engine.prepare(rate, channels), "oversampled engine preparation");
                const int delay = engine.latencySamples();
                if (channels == 1 && block == 1)
                    std::cout << "MEASURE precision=" << sizeof(T) * 8 << " rate=" << rate
                              << " factor=" << engine.oversamplingFactor() << " latency=" << delay
                              << '\n';
                check(delay >= 0 && delay < 512 &&
                          engine.oversamplingFactor() == (rate <= 48000   ? 4
                                                          : rate <= 96000 ? 2
                                                                          : 1),
                      "quality factor and bounded integer latency");
                std::vector<T> left(size_t(block), T{}), right(size_t(block), T{});
                T *audio[]{left.data(), right.data()};
                bool exact = true;
                int sample = 0;
                for (int iteration = 0; iteration < 3 + (delay + 1) / block; ++iteration)
                {
                    for (int i = 0; i < block; ++i)
                    {
                        left[size_t(i)] = T(.25 * std::sin((sample + i) * .13));
                        right[size_t(i)] = -left[size_t(i)];
                    }
                    engine.process(audio, channels, block, {});
                    for (int i = 0; i < block; ++i)
                    {
                        const int index = sample + i - delay;
                        const T expected = index >= 0 ? T(.25 * std::sin(index * .13)) : T(0);
                        exact = exact && left[size_t(i)] == expected &&
                                (channels == 1 || right[size_t(i)] == -expected);
                    }
                    sample += block;
                }
                check(exact, "Off path exact delayed unity across block boundaries");
                aurum::Parameters p;
                p.delta = true;
                engine.prepare(rate, channels, p);
                std::fill(left.begin(), left.end(), T(.2));
                std::fill(right.begin(), right.end(), T(-.2));
                engine.process(audio, channels, block, p);
                check(std::all_of(left.begin(), left.end(), [](T v) { return v == 0; }),
                      "Off Delta is an exact aligned null");
                p.delta = false;
                p.clipMode = 1;
                p.inputDb = 12;
                p.ceilingDb = -18;
                engine.prepare(rate, channels, p);
                bool bypassExact = true;
                sample = 0;
                for (int iteration = 0; iteration < 3 + (delay + 1) / block; ++iteration)
                {
                    std::fill(left.begin(), left.end(), T(.375));
                    std::fill(right.begin(), right.end(), T(-.375));
                    engine.process(audio, channels, block, p, nullptr, true);
                    for (int i = 0; i < block; ++i)
                        bypassExact = bypassExact &&
                                      left[size_t(i)] == (sample + i >= delay ? T(.375) : T(0));
                    sample += block;
                }
                check(bypassExact, "host bypass retains unity and PDC while clipping engaged");
            }
    for (int scenario = 0; scenario < 5; ++scenario)
    {
        aurum::Parameters p;
        p.inputDb = 6;
        p.outputDb = -3;
        p.blend = .3;
        if (scenario == 1)
        {
            p.linked = false;
            p.wetDb = -6;
            p.dryDb = -9;
        }
        if (scenario == 2)
        {
            p.delta = true;
            p.wetMute = true;
            p.dryMute = true;
        }
        if (scenario == 3)
            p.wetMute = true;
        if (scenario == 4)
            p.bypass = true;
        aurum::OversampledEngine<T> engine;
        engine.prepare(48000, 1, p);
        std::array<T, 4096> samples{};
        samples.fill(T(.2));
        T *audio[]{samples.data()};
        engine.process(audio, 1, int(samples.size()), p);
        const auto mix = aurum::ParallelMixer::targets(p);
        const double expected =
            p.bypass ? double(T(.2))
                     : aurum::ParallelMixer::combine(double(T(.2)) * aurum::gain(6), double(T(.2)),
                                                     mix, p.delta ? 1 : 0) *
                           aurum::gain(-3);
        check(std::abs(double(samples.back()) - expected) < (sizeof(T) == 4 ? 1e-7 : 1e-14),
              "prepared oversampled Off path preserves trim, mixer, mute, Delta and plugin bypass "
              "conventions");
    }
    for (int mode : {1, 2, 3})
    {
        aurum::Parameters p;
        p.clipMode = mode;
        p.ceilingDb = -6;
        aurum::OversampledEngine<T> engine;
        engine.prepare(48000, 2, p);
        std::array<T, 257> left{}, right{};
        T *audio[]{left.data(), right.data()};
        bool finite = true, symmetric = true;
        double peak = 0;
        for (int block = 0; block < 200; ++block)
        {
            for (int i = 0; i < 257; ++i)
            {
                left[size_t(i)] = T(2 * std::sin((block * 257 + i) * .21));
                right[size_t(i)] = -left[size_t(i)];
            }
            if (block == 10)
                left[0] = std::numeric_limits<T>::quiet_NaN();
            if (block == 11)
                right[0] = std::numeric_limits<T>::infinity();
            engine.process(audio, 2, 257, p);
            for (int i = 0; i < 257; ++i)
            {
                finite =
                    finite && std::isfinite(left[size_t(i)]) && std::isfinite(right[size_t(i)]);
                if (block > 20)
                    symmetric =
                        symmetric && std::abs(double(left[size_t(i)] + right[size_t(i)])) < 1e-6;
                peak = std::max(peak, std::abs(double(left[size_t(i)])));
            }
        }
        check(finite && symmetric, "clipping is finite and preserves anti-phase symmetry");
        check(peak > .49 && peak < .8,
              "clipped output bounded with explicitly allowed FIR overshoot");
        std::cout << "MEASURE mode=" << mode << " reconstructed_peak=" << peak
                  << " ceiling=" << aurum::gain(-6) << '\n';
    }
}
template <class Check> void transitionAndImpulseTests(Check check)
{
    for (double rate : {44100., 48000., 88200., 96000., 192000.})
    {
        aurum::OversampledEngine<double> engine;
        aurum::Parameters p;
        p.clipMode = 1;
        engine.prepare(rate, 1, p);
        std::array<double, 2048> samples{};
        samples[256] = .001;
        double *audio[]{samples.data()};
        engine.process(audio, 1, int(samples.size()), p);
        const auto peak = std::max_element(samples.begin(), samples.end());
        const int index = int(peak - samples.begin());
        check(std::abs(index - 256 - engine.latencySamples()) <= 1,
              "filtered impulse peak agrees with reported latency within one sample");
    }
    aurum::OversampledEngine<double> engine;
    aurum::Parameters p;
    engine.prepare(48000, 1, p);
    std::array<double, 257> samples{};
    double *audio[]{samples.data()};
    double error = 0;
    for (int block = 0; block < 100; ++block)
    {
        if (block == 10)
            p.clipMode = 1;
        if (block == 25)
            p.clipMode = 2;
        if (block == 40)
            p.clipMode = 3;
        if (block == 55)
            p.clipMode = 0;
        samples.fill(.01);
        engine.process(audio, 1, int(samples.size()), p);
        if (block > 3)
            for (double sample : samples)
                error = std::max(error, std::abs(sample - .01));
    }
    std::cout << "MEASURE Off/Hard/Classic/Modern transition DC error=" << error << '\n';
    check(error < 1e-5, "linear-region mode changes preserve DC without an engagement gain dip");
    p.clipMode = 1;
    engine.prepare(48000, 1, p);
    samples.fill(.25);
    const auto start = std::chrono::steady_clock::now();
    for (int block = 0; block < 1000; ++block)
        engine.process(audio, 1, int(samples.size()), p);
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "MEASURE local 4x mono double 257000 samples elapsed=" << seconds
              << " s; audio_duration=" << 257000. / 48000
              << " s (informational, not a real-time CPU budget)\n";
}
template <class Check> void filterDesignTests(Check check)
{
    for (int stage = 0; stage < 2; ++stage)
    {
        auto up = juce::dsp::FilterDesign<double>::designFIRLowpassHalfBandEquirippleMethod(
            stage == 0 ? .05 : .10, stage == 0 ? -90 : -80);
        auto down = juce::dsp::FilterDesign<double>::designFIRLowpassHalfBandEquirippleMethod(
            stage == 0 ? .06 : .12, stage == 0 ? -75 : -65);
        std::cout << "MEASURE FIR stage=" << stage + 1 << " up_taps=" << up->getFilterOrder() + 1
                  << " down_taps=" << down->getFilterOrder() + 1 << '\n';
        check(up->getFilterOrder() > 0 && down->getFilterOrder() > 0,
              "actual FIR coefficient lengths available");
    }
}
template <class Check> void aliasTest(Check check)
{
    constexpr int length = 8192, warmup = 4096;
    aurum::Parameters p;
    p.clipMode = 1;
    p.ceilingDb = -6;
    aurum::OversampledEngine<double> engine;
    engine.prepare(48000, 1, p);
    std::vector<double> audio(size_t(length + warmup), 0);
    for (int i = 0; i < length + warmup; ++i)
        audio[size_t(i)] = 2 * std::sin(juce::MathConstants<double>::twoPi * 9000 * i / 48000);
    double *pointers[]{audio.data()};
    engine.process(pointers, 1, int(audio.size()), p);
    double re = 0, im = 0, baseRe = 0, baseIm = 0;
    for (int i = 0; i < length; ++i)
    {
        const double angle = juce::MathConstants<double>::twoPi * 21000 * i / 48000;
        const double original =
            aurum::clipSample(2 * std::sin(juce::MathConstants<double>::twoPi * 9000 * i / 48000),
                              aurum::gain(-6), aurum::ClipMode::hard);
        re += audio[size_t(i + warmup)] * std::cos(angle);
        im += audio[size_t(i + warmup)] * std::sin(angle);
        baseRe += original * std::cos(angle);
        baseIm += original * std::sin(angle);
    }
    const double suppression =
        20 * std::log10(std::hypot(baseRe, baseIm) / std::max(1e-20, std::hypot(re, im)));
    std::cout << "MEASURE 9kHz hard-clipped 21kHz alias suppression versus 1x=" << suppression
              << " dB\n";
    check(suppression > 20, "4x FIR suppresses measured folded third harmonic by at least 20 dB");
}
} // namespace clipping_tests
