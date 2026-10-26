#include "dsp/ClipperEngine.h"
#include "dsp/ProcessingEngine.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <random>
namespace
{
bool watch = false;
unsigned allocations = 0;
int checks = 0, failures = 0;
void check(bool pass, const char *name)
{
    ++checks;
    if (!pass)
    {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}
void near(double actual, double expected, double tolerance, const char *name)
{
    check(std::abs(actual - expected) <= tolerance, name);
}
} // namespace
void *operator new(std::size_t n)
{
    if (watch)
        ++allocations;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept
{
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept
{
    std::free(p);
}
void *operator new[](std::size_t n)
{
    return ::operator new(n);
}
void operator delete[](void *p) noexcept
{
    ::operator delete(p);
}
void operator delete[](void *p, std::size_t) noexcept
{
    ::operator delete(p);
}
template <class T> void routing()
{
    constexpr int capacity = 4096;
    std::array<T, capacity> left{}, right{};
    T *audio[]{left.data(), right.data()};
    for (double rate : {44100., 48000., 88200., 96000., 192000.})
        for (unsigned channels : {1u, 2u})
            for (int block : {1, 17, 64, 257, 4096})
            {
                aurum::ProcessingEngine engine;
                check(engine.prepare(rate, channels), "prepare accepted supported configuration");
                for (int i = 0; i < block; ++i)
                {
                    left[i] = T(.2 * std::sin(i * .11));
                    right[i] = -left[i];
                }
                auto a = left, b = right;
                engine.process(audio, channels, block, {});
                check(left == a && (channels == 1 || right == b),
                      "default unity and no stereo crosstalk");
                aurum::Parameters p;
                p.inputDb = 6;
                p.outputDb = -3;
                engine.reset(p);
                left.fill(T(.1));
                right.fill(T(-.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1 * aurum::gain(3), 1e-7, "wet input and final output gain");
                p.blend = 0;
                engine.reset(p);
                left.fill(T(.1));
                right.fill(T(-.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1 * aurum::gain(-3), 1e-7, "dry is original input before trim");
                p.outputDb = 0;
                p.blend = .25;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1 * (.25 * aurum::gain(6) + .75), 1e-7, "linked linear blend");
                p.linked = false;
                p.wetDb = 0;
                p.dryDb = 0;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1 * (aurum::gain(6) + 1), 1e-7, "independent dB parallel summing");
                p.wetMute = true;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1, 1e-7, "wet mute");
                p.dryMute = true;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], 0, 0, "both paths muted");
                p.delta = true;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], .1 * (aurum::gain(6) - 1), 1e-7,
                     "delta is wet minus aligned original, independent of mutes");
                p.inputDb = 0;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                near(left[0], 0, 0, "identical paths delta null");
                p.bypass = true;
                p.inputDb = 12;
                p.outputDb = 12;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p);
                check(left[0] == T(.1), "settled plugin bypass is bit-exact dry unity");
                p.bypass = false;
                engine.reset(p);
                left.fill(T(.1));
                engine.process(audio, channels, block, p, nullptr, true);
                near(left[0], .1, 1e-7, "host bypass dry unity");
                check(engine.latencySamples() == 0, "zero foundation latency");
            }
}
void smoothingAndSafety()
{
    aurum::ProcessingEngine engine;
    engine.prepare(48000, 2);
    std::array<double, 4096> l{}, r{};
    l.fill(.1);
    r.fill(-.1);
    double *audio[]{l.data(), r.data()};
    aurum::Parameters p;
    p.inputDb = 12;
    aurum::Telemetry meter;
    watch = true;
    engine.process(audio, 2, 4096, p, &meter);
    watch = false;
    check(allocations == 0, "audio processing performs no heap allocations including telemetry");
    double maximumStep = 0;
    for (int i = 1; i < 960; ++i)
        maximumStep = std::max(maximumStep, std::abs(l[i] - l[i - 1]));
    check(maximumStep < .0004, "20 ms ramp bounds adjacent changes");
    near(l[959], .1 * aurum::gain(12), 1e-12, "ramp reaches target in 960 samples");
    p.bypass = true;
    l.fill(.1);
    r.fill(-.1);
    engine.process(audio, 2, 4096, p);
    near(l[959], .1, 1e-12, "bypass crossfade settles in 20 ms");
    p = {};
    engine.reset(p);
    l.fill(.2);
    r.fill(-.2);
    l[0] = std::numeric_limits<double>::infinity();
    l[1] = std::numeric_limits<double>::quiet_NaN();
    engine.process(audio, 2, 4096, p);
    check(l[0] == 0 && l[1] == 0 && std::isfinite(r[0]), "NaN and Inf sanitised independently");
    p.inputDb = std::numeric_limits<double>::quiet_NaN();
    engine.reset(p);
    l.fill(.2);
    engine.process(audio, 2, 1, p);
    near(l[0], .2, 1e-12, "invalid parameter falls back safely");
    p = {};
    p.linked = false;
    p.wetDb = p.dryDb = 6;
    engine.reset(p);
    l.fill(1);
    engine.process(audio, 2, 1, p);
    check(l[0] > 3.9, "no hidden limiter on independent sum");
    check(!engine.prepare(0, 2) && !engine.prepare(48000, 3), "invalid preparation rejected");
}
void telemetry()
{
    aurum::Telemetry t;
    double in[]{.1, -.2}, out[]{.3, -1.2};
    for (int i = 0; i < 64; ++i)
        t.push(in, out, 2);
    aurum::Envelope e;
    check(t.pop(e) && !t.pop(e), "one envelope per 64 samples");
    near(e.inputMin, -.2, 1e-7, "envelope preserves negative peak");
    near(e.outputMax, .3, 1e-7, "envelope preserves positive peak");
    auto p = t.consumePeaks();
    check(p.overloaded && p.input[1] > .19 && p.output[1] > 1.19,
          "real peaks and overload telemetry");
    check(!t.consumePeaks().overloaded, "overload event consumed");
    for (int i = 0; i < 64 * 1100; ++i)
        t.push(in, out, 2);
    check(t.consumePeaks().droppedFrames > 0,
          "bounded queue safely drops graphics when editor closed");
    int count = 0;
    while (t.pop(e))
        ++count;
    check(count == 1024, "queue capacity remains bounded");
    double positive[]{.5, .7};
    for (int i = 0; i < 64; ++i)
        t.push(positive, positive, 1);
    t.pop(e);
    near(e.inputMin, .5, 1e-7, "envelope minimum retains DC offset");
}
void clipCurves()
{
    for (auto mode : {aurum::ClipMode::hard, aurum::ClipMode::classic, aurum::ClipMode::modern})
        for (double db : {-36., -18., -6., 0.})
        {
            const double limit = aurum::gain(db);
            double previous = 0;
            bool monotonic = true, odd = true, bounded = true, identity = true;
            for (int i = 0; i <= 20000; ++i)
            {
                const double x = 2.0 * limit * i / 20000;
                const double y = aurum::clipSample(x, limit, mode);
                monotonic = monotonic && y + 1e-14 >= previous;
                odd = odd && std::abs(y + aurum::clipSample(-x, limit, mode)) < 1e-14;
                bounded = bounded && y <= limit + 1e-14;
                if (x <= limit * .49)
                    identity = identity && y == x;
                previous = y;
            }
            check(monotonic && odd && bounded && identity,
                  "curve monotonicity, odd symmetry, bounds and linear region");
            near(aurum::clipSample(10 * limit, limit, mode), limit, 1e-14,
                 "curve saturates at requested ceiling");
        }
    check(aurum::clipSample(.9, 1, aurum::ClipMode::classic) !=
              aurum::clipSample(.9, 1, aurum::ClipMode::modern),
          "Classic and Modern use distinct knees");
    near(aurum::clipSample(.9, 1, aurum::ClipMode::hard), .9, 0, "hard clip exact below threshold");
    near(aurum::clipSample(2, 1, aurum::ClipMode::off), 2, 0, "Off never clips");
    near(aurum::clipSample(std::numeric_limits<double>::infinity(), 1, aurum::ClipMode::hard), 0, 0,
         "invalid clip input sanitised");
    double harmonics[2][2]{};
    for (int family = 0; family < 2; ++family)
        for (int n = 0; n < 8192; ++n)
        {
            const double phase = 2 * std::acos(-1.) * n / 8192;
            const double output =
                aurum::clipSample(1.2 * std::sin(phase), 1,
                                  family == 0 ? aurum::ClipMode::classic : aurum::ClipMode::modern);
            harmonics[family][0] += output * std::sin(3 * phase) * 2 / 8192;
            harmonics[family][1] += output * std::sin(5 * phase) * 2 / 8192;
        }
    check(std::abs(harmonics[0][0] - harmonics[1][0]) > .001 &&
              std::abs(harmonics[0][1] - harmonics[1][1]) > .001,
          "Classic and Modern have distinct third/fifth harmonic characteristics");
    std::cout << "MEASURE Classic H3=" << harmonics[0][0] << " H5=" << harmonics[0][1]
              << "; Modern H3=" << harmonics[1][0] << " H5=" << harmonics[1][1] << '\n';
    for (auto mode : {aurum::ClipMode::classic, aurum::ClipMode::modern})
    {
        const double knee = mode == aurum::ClipMode::classic ? .8 : .5, high = 2 - knee, h = 1e-5;
        near((aurum::clipSample(knee + h, 1, mode) - aurum::clipSample(knee - h, 1, mode)) /
                 (2 * h),
             1, 1e-7, "soft knee onset slope continuous");
        near((aurum::clipSample(high + h, 1, mode) - aurum::clipSample(high - h, 1, mode)) /
                 (2 * h),
             0, 1e-7, "soft knee saturation slope continuous");
    }
}
int main()
{
    routing<float>();
    routing<double>();
    smoothingAndSafety();
    telemetry();
    clipCurves();
    std::cout << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
