#pragma once
#include "dsp/QualityBank.h"
#include <chrono>
#include <fstream>
namespace measurements
{
inline void wav(const juce::File &file, const std::vector<float> &stereo)
{
    std::ofstream out(file.getFullPathName().toStdString(), std::ios::binary);
    auto word = [&](uint32_t value, int bytes)
    {
        for (int n = 0; n < bytes; ++n)
            out.put(char((value >> (8 * n)) & 255));
    };
    const auto bytes = uint32_t(stereo.size() * sizeof(float));
    out.write("RIFF", 4);
    word(bytes + 36, 4);
    out.write("WAVEfmt ", 8);
    word(16, 4);
    word(3, 2);
    word(2, 2);
    word(48000, 4);
    word(48000 * 8, 4);
    word(8, 2);
    word(32, 2);
    out.write("data", 4);
    word(bytes, 4);
    out.write(reinterpret_cast<const char *>(stereo.data()), bytes);
}
inline void exportAll(const juce::File &directory)
{
    directory.createDirectory();
    const auto path = [&](const char *name)
    { return directory.getChildFile(name).getFullPathName().toStdString(); };
    std::ofstream curves(path("transfer.csv"));
    curves << "input,linear,hard,classic,modern,goldModern,goldClassic,goldUnity\n";
    for (int n = -1000; n <= 1000; ++n)
    {
        const double x = n * .0015;
        curves << x << ',' << x;
        for (auto mode : {aurum::ClipMode::hard, aurum::ClipMode::classic, aurum::ClipMode::modern})
            curves << ',' << aurum::clipSample(x, .5, mode);
        curves << ',' << aurum::GoldProcessor::process(x, .5, 2.5, 0, false) << ','
               << aurum::GoldProcessor::process(x, .5, 6, 1, false) << ','
               << aurum::GoldProcessor::process(x, .5, 6, 1, true) << '\n';
    }
    std::ofstream sweep(path("amplitude-sweep.csv"));
    sweep << "input_db,hard_db,classic_db,modern_db,gold_modern_db,gold_classic_db\n";
    for (int n = 0; n <= 720; ++n)
    {
        const double db = -60 + n * .1, x = aurum::gain(db);
        sweep << db;
        for (auto mode : {aurum::ClipMode::hard, aurum::ClipMode::classic, aurum::ClipMode::modern})
            sweep << ',' << 20 * std::log10(aurum::clipSample(x, .5, mode));
        sweep << ',' << 20 * std::log10(aurum::GoldProcessor::process(x, .5, 2.5, 0, false)) << ','
              << 20 * std::log10(aurum::GoldProcessor::process(x, .5, 6, 1, false)) << '\n';
    }
    std::ofstream alias(path("alias.csv"));
    alias << "phase,quality,tone_hz,folded_hz,suppression_db,reconstructed_peak,elapsed_seconds\n";
    constexpr int count = 12288, warm = 4096;
    for (int phase = 0; phase < 2; ++phase)
        for (int quality = 0; quality < 3; ++quality)
            for (double hz : {9000., 10000., 11000.})
            {
                aurum::Parameters p;
                p.clipMode = 1;
                p.ceilingDb = -6;
                p.peakGuard = false;
                p.filterPhase = phase;
                p.quality = quality;
                aurum::QualityBank<double> engine;
                engine.prepare(48000, 1, p);
                std::vector<double> values(size_t(count + warm), 0);
                for (size_t i = 0; i < values.size(); ++i)
                    values[i] = 2 * std::sin(2 * std::acos(-1.) * hz * i / 48000);
                double *audio[]{values.data()};
                const auto start = std::chrono::steady_clock::now();
                engine.process(audio, 1, int(values.size()), p);
                const double elapsed =
                    std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                double re = 0, im = 0, baseRe = 0, baseIm = 0, peak = 0;
                const double folded = 48000 - 3 * hz;
                for (int i = 0; i < count; ++i)
                {
                    const double angle = 2 * std::acos(-1.) * folded * i / 48000;
                    const double base =
                        aurum::clipSample(2 * std::sin(2 * std::acos(-1.) * hz * i / 48000),
                                          aurum::gain(-6), aurum::ClipMode::hard);
                    re += values[size_t(i + warm)] * std::cos(angle);
                    im += values[size_t(i + warm)] * std::sin(angle);
                    baseRe += base * std::cos(angle);
                    baseIm += base * std::sin(angle);
                    peak = std::max(peak, std::abs(values[size_t(i + warm)]));
                }
                alias << phase << ',' << quality << ',' << hz << ',' << folded << ','
                      << 20 * std::log10(std::max(1e-20, std::hypot(baseRe, baseIm)) /
                                         std::max(1e-20, std::hypot(re, im)))
                      << ',' << peak << ',' << elapsed << '\n';
            }
    std::ofstream response(path("box-tone.csv"));
    response << "rate,mode,hz,db\n";
    for (double rate : {44100., 48000., 96000.})
        for (int mode : {0, 1, 2})
        {
            aurum::BoxToneProcessor tone;
            tone.prepare(rate, true, mode);
            std::array<double, 512> impulse{};
            impulse[0] = 1;
            for (double &x : impulse)
            {
                double frame[]{x};
                tone.process(frame, 1, mode);
                x = frame[0];
            }
            for (double hz = 100; hz < std::min(24000., rate * .5); hz += 100)
            {
                double re = 0, im = 0;
                for (size_t i = 0; i < impulse.size(); ++i)
                {
                    const double angle = 2 * std::acos(-1.) * hz * i / rate;
                    re += impulse[i] * std::cos(angle);
                    im += impulse[i] * std::sin(angle);
                }
                response << rate << ',' << mode << ',' << hz << ','
                         << 20 * std::log10(std::hypot(re, im)) << '\n';
            }
        }
    std::ofstream alchemy(path("alchemy.csv"));
    alchemy << "hz,amplitude,attenuation_db\n";
    for (double hz : {500., 1000., 3000., 5000., 8000., 10000., 16000., 20000.})
        for (double amplitude : {.05, .25, .5, .98})
        {
            aurum::AlchemyProcessor effect;
            effect.prepare(192000);
            double before = 0, after = 0;
            for (int i = 0; i < 96000; ++i)
            {
                const double x = amplitude * std::sin(2 * std::acos(-1.) * hz * i / 192000);
                double frame[]{x};
                effect.process(frame, 1, 1, 2);
                if (i > 19200)
                {
                    before += x * x;
                    after += frame[0] * frame[0];
                }
            }
            alchemy << hz << ',' << amplitude << ',' << 10 * std::log10(after / before) << '\n';
        }
    std::ofstream harmonics(path("harmonics.csv"));
    harmonics << "mode,tone_hz,harmonic,amplitude\n";
    for (int mode = 0; mode < 5; ++mode)
        for (double hz : {100., 1000., 8000.})
            for (int harmonic = 1; harmonic <= 15 && hz * harmonic < 24000; ++harmonic)
            {
                double re = 0, im = 0;
                constexpr int n = 48000;
                for (int i = 0; i < n; ++i)
                {
                    const double phase = 2 * std::acos(-1.) * hz * i / 48000,
                                 x = 1.2 * std::sin(phase);
                    const double y = mode < 3 ? aurum::clipSample(x, 1, aurum::ClipMode(mode + 1))
                                              : aurum::GoldProcessor::process(
                                                    x, 1, mode == 3 ? 2.5 : 6, mode - 3, false);
                    re += y * std::cos(harmonic * phase);
                    im += y * std::sin(harmonic * phase);
                }
                harmonics << mode << ',' << hz << ',' << harmonic << ','
                          << 2 * std::hypot(re, im) / n << '\n';
            }
    uint32_t seed = 1234567;
    auto noise = [&]()
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return double(seed) / 4294967295. * 2 - 1;
    };
    std::ofstream summary(path("signals.csv"));
    summary << "signal,input_lufs,output_lufs,input_peak,output_peak\n";
    for (int signal = 0; signal < 4; ++signal)
    {
        constexpr int samples = 240000;
        std::vector<float> original(size_t(samples * 2), 0), processed(size_t(samples * 2), 0);
        aurum::LoudnessMeter inputMeter, outputMeter;
        inputMeter.prepare(48000);
        outputMeter.prepare(48000);
        aurum::Parameters p;
        p.clipMode = 2;
        p.ceilingDb = -1;
        p.goldMode = 1;
        p.goldDb = 3;
        p.alchemyDb = 1.5;
        p.boxTone = 2;
        p.quality = 1;
        aurum::QualityBank<float> engine;
        engine.prepare(48000, 2, p, true);
        double inputPeak = 0, outputPeak = 0;
        for (int offset = 0; offset < samples; offset += 256)
        {
            std::array<float, 256> l{}, r{};
            const int n = std::min(256, samples - offset);
            for (int i = 0; i < n; ++i)
            {
                const double t = (offset + i) / 48000., beat = std::fmod(t, .5),
                             kick = .8 * std::exp(-beat * 35) *
                                    std::sin(2 * std::acos(-1.) *
                                             (55 * beat + 1.2 * (1 - std::exp(-beat * 50)))),
                             hat = noise() * .10 * std::exp(-std::fmod(t, .125) * 80);
                double x = 0, y = 0;
                if (signal == 0)
                    x = y = aurum::gain(-23) * std::sin(2 * std::acos(-1.) * 1000 * t);
                if (signal == 1)
                {
                    x = kick + hat;
                    y = kick - hat;
                }
                if (signal == 2)
                {
                    x = .25 * std::sin(2 * std::acos(-1.) * 110 * t) +
                        .1 * std::sin(2 * std::acos(-1.) * 440 * t) + kick + hat;
                    y = .25 * std::sin(2 * std::acos(-1.) * 110 * t) +
                        .1 * std::sin(2 * std::acos(-1.) * 659 * t) + kick - hat;
                }
                if (signal == 3)
                {
                    x = .15 * noise();
                    y = .15 * noise();
                }
                l[size_t(i)] = float(x);
                r[size_t(i)] = float(y);
                original[size_t((offset + i) * 2)] = float(x);
                original[size_t((offset + i) * 2 + 1)] = float(y);
                double frame[]{x, y};
                inputMeter.push(frame, 2);
                inputPeak = std::max({inputPeak, std::abs(x), std::abs(y)});
            }
            float *audio[]{l.data(), r.data()};
            engine.process(audio, 2, n, p);
            for (int i = 0; i < n; ++i)
            {
                processed[size_t((offset + i) * 2)] = l[size_t(i)];
                processed[size_t((offset + i) * 2 + 1)] = r[size_t(i)];
                double frame[]{double(l[size_t(i)]), double(r[size_t(i)])};
                outputMeter.push(frame, 2);
                outputPeak = std::max({outputPeak, std::abs(frame[0]), std::abs(frame[1])});
            }
        }
        const juce::String name = signal == 0   ? "sine"
                                  : signal == 1 ? "drums"
                                  : signal == 2 ? "synthetic-mix"
                                                : "noise";
        wav(directory.getChildFile(name + "-input.wav"), original);
        wav(directory.getChildFile(name + "-output.wav"), processed);
        summary << name << ',' << inputMeter.shortTerm() << ',' << outputMeter.shortTerm() << ','
                << inputPeak << ',' << outputPeak << '\n';
    }
}
} // namespace measurements
