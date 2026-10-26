#pragma once
#include "dsp/AlchemyProcessor.h"
#include "dsp/BoxToneProcessor.h"
#include "dsp/GoldProcessor.h"
#include "dsp/LoudnessMeter.h"
#include "dsp/ReconstructionGuard.h"
#include <iostream>
#include <vector>
namespace product_tests
{
template <class Check> void curves(Check check)
{
    for (int mode : {0, 1})
        for (double amount : {0., .5, 1.5, 2.5, 6.})
            for (double ceiling : {.0158489319, .25, 1.})
            {
                const double selected = std::min(amount, mode == 0 ? 2.5 : 6.);
                bool monotonic = true, bounded = true, symmetric = true;
                double previous = 0;
                for (int n = 0; n <= 10000; ++n)
                {
                    const double x = ceiling * n / 10000., y = aurum::GoldProcessor::process(
                                                               x, ceiling, selected, mode, false);
                    monotonic = monotonic && y + 1e-14 >= previous;
                    bounded = bounded && y <= ceiling + 1e-14;
                    symmetric = symmetric &&
                                std::abs(y + aurum::GoldProcessor::process(-x, ceiling, selected,
                                                                           mode, false)) < 1e-14;
                    previous = y;
                }
                check(monotonic && bounded && symmetric,
                      "Gold monotonicity, symmetry and peak preservation across amounts/ceilings");
                check(std::abs(
                          aurum::GoldProcessor::process(ceiling, ceiling, selected, mode, false) -
                          ceiling) < 1e-14,
                      "Gold ceiling anchor exact");
                const double h = ceiling * 1e-5;
                const double second =
                    (aurum::GoldProcessor::process(ceiling + h, ceiling, selected, mode, false) -
                     2 * ceiling +
                     aurum::GoldProcessor::process(ceiling - h, ceiling, selected, mode, false)) /
                    (h * h);
                check(std::abs(second * ceiling) < .001,
                      "Gold curvature joins the linear near-ceiling continuation smoothly");
                const double tiny = ceiling * 1e-8;
                check(std::abs(aurum::GoldProcessor::process(tiny, ceiling, selected, mode, false) /
                                   tiny -
                               aurum::gain(selected)) < 1e-6,
                      "Gold low-level gain matches amount");
                check(std::abs(aurum::GoldProcessor::process(tiny, ceiling, selected, mode, true) /
                                   tiny -
                               1) < 1e-6,
                      "Gold Unity compensation approaches unity at low levels");
            }
    check(std::abs(aurum::GoldProcessor::process(.5, 1, 2, 0, false) -
                   aurum::GoldProcessor::process(.5, 1, 2, 1, false)) > .01,
          "Gold curve families differ at the same enhancement");
}
template <class Check> void metering(Check check)
{
    for (double rate : {44100., 48000., 88200., 96000., 192000.})
        for (unsigned channels : {1u, 2u})
        {
            aurum::LoudnessMeter meter;
            meter.prepare(rate);
            double frame[2]{};
            for (int i = 0; i < int(rate * 2.9); ++i)
                meter.push(frame, channels);
            check(std::isnan(meter.shortTerm()),
                  "short-term loudness unavailable before three seconds");
            for (int i = 0; i < int(rate * .2); ++i)
                meter.push(frame, channels);
            check(std::isinf(meter.shortTerm()) && meter.shortTerm() < 0,
                  "three-second silence is valid minus infinity");
            for (int i = 0; i < int(rate * 4); ++i)
            {
                frame[0] = frame[1] =
                    aurum::gain(-23) * std::sin(2 * std::acos(-1.) * 1000 * i / rate);
                meter.push(frame, channels);
            }
            const double expected = channels == 2 ? -23.0 : -26.01;
            std::cout << "MEASURE LUFS rate=" << rate << " channels=" << channels
                      << " value=" << meter.shortTerm() << '\n';
            check(std::abs(meter.shortTerm() - expected) < .10,
                  "K-weighted steady sine loudness within 0.10 LU of BS.1770 reference target");
            meter.reset();
            check(std::isnan(meter.shortTerm()), "loudness reset clears startup window");
        }
}
template <class Check> void contours(Check check)
{
    for (double rate : {44100., 48000., 96000.})
    {
        for (int mode : {0, 1, 2})
        {
            aurum::BoxToneProcessor tone;
            tone.prepare(rate, true, mode);
            std::vector<double> impulse(2048, 0);
            impulse[0] = 1;
            for (auto &x : impulse)
            {
                double frame[]{x};
                tone.process(frame, 1, mode);
                x = frame[0];
            }
            auto response = [&](double frequency)
            {
                double re = 0, im = 0;
                for (size_t i = 0; i < impulse.size(); ++i)
                {
                    const double phase = 2 * std::acos(-1.) * frequency * i / rate;
                    re += impulse[i] * std::cos(phase);
                    im += impulse[i] * std::sin(phase);
                }
                return 20 * std::log10(std::hypot(re, im));
            };
            check(std::abs(response(100)) < .02 && std::abs(response(1000)) < .04,
                  "Box Tone substantially flat below 1 kHz");
            if (mode == 0)
                check(impulse[64] == 1 && std::count(impulse.begin(), impulse.end(), 0.) == 2047,
                      "Flat Box Tone is an exact delayed bypass");
            std::cout << "MEASURE Box rate=" << rate << " mode=" << mode << " 3k=" << response(3000)
                      << " 8k=" << response(8000) << " 20k=" << response(20000) << '\n';
            if (mode == 2)
                check(response(20000) < -.15 && response(20000) > -.55,
                      "Modern Box Tone measured 20 kHz contour is subtle and negative");
        }
        auto attenuation = [&](double amplitude)
        {
            aurum::AlchemyProcessor filter;
            filter.prepare(rate * 4);
            double input = 0, output = 0;
            for (int i = 0; i < int(rate * 4 * .5); ++i)
            {
                const double x = amplitude * std::sin(2 * std::acos(-1.) * 8000 * i / (rate * 4));
                double frame[]{x, -x};
                filter.process(frame, 2, 1, 2);
                if (i > int(rate * 4 * .1))
                {
                    input += x * x;
                    output += frame[0] * frame[0];
                }
                if (std::abs(frame[0] + frame[1]) > 1e-10)
                    check(false, "Alchemy anti-phase symmetry");
            }
            return 10 * std::log10(output / input);
        };
        const double quiet = attenuation(.05), hot = attenuation(.98);
        std::cout << "MEASURE Alchemy rate=" << rate << " quiet=" << quiet << " hot=" << hot
                  << '\n';
        check(std::abs(quiet) < .001 && hot < -.4 && hot > -2.1,
              "Alchemy responds dynamically to level rather than acting as fixed EQ");
    }
    aurum::ReconstructionGuard guard;
    guard.prepare(48000);
    double maximum = 0;
    for (int i = 0; i < 4096; ++i)
    {
        double f[]{i % 37 == 0 ? 2. : .4, -.3};
        guard.process(f, 2, .5, true);
        maximum = std::max(maximum, std::abs(f[0]));
    }
    check(maximum <= .5 + 1e-12, "wet reconstruction guard bounds linked output sample peaks");
}
} // namespace product_tests
