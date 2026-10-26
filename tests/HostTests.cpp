#include "AllocationProbe.h"
#include "ClippingTests.h"
#include "MeasurementExport.h"
#include "plugin/PluginProcessor.h"
#include <iostream>
#include <thread>
namespace
{
int checks = 0, failures = 0;
void check(bool pass, const char *label)
{
    ++checks;
    std::cout << (pass ? "PASS " : "FAIL ") << label << std::endl;
    if (!pass)
        ++failures;
}
void set(aurum::PluginProcessor &p, const char *id, float value)
{
    auto *parameter = p.parameters.state.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
} // namespace
int main(int argc, char **argv)
{
#if defined(_WIN32)
    _putenv_s("AURUM_NO_SETTINGS", "1");
#else
    setenv("AURUM_NO_SETTINGS", "1", 1);
#endif
    juce::ScopedJuceInitialiser_GUI gui;
    const auto testPresetDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                         .getNonexistentChildFile("AurumPresets", "", false);
#if defined(_WIN32)
    _putenv_s("AURUM_PRESET_DIR", testPresetDirectory.getFullPathName().toRawUTF8());
#else
    setenv("AURUM_PRESET_DIR", testPresetDirectory.getFullPathName().toRawUTF8(), 1);
#endif
    // Processors must outlive all editors and parameter attachments.
    auto master = std::make_unique<aurum::PluginProcessor>(false),
         restored = std::make_unique<aurum::PluginProcessor>(false),
         track = std::make_unique<aurum::PluginProcessor>(true);
    clipping_tests::engineTests<float>(check);
    clipping_tests::engineTests<double>(check);
    clipping_tests::transitionAndImpulseTests(check);
    clipping_tests::filterDesignTests(check);
    clipping_tests::aliasTest(check);
    {
        aurum::OversampledEngine<double> engine;
        aurum::Parameters p;
        p.clipMode = 1;
        engine.prepare(48000, 2, p);
        std::array<double, 4096> left{}, right{};
        double *audio[]{left.data(), right.data()};
        left.fill(.5);
        right.fill(-.5);
        allocation_probe::count = 0;
        allocation_probe::enabled = true;
        for (int mode = 0; mode < 4; ++mode)
        {
            p.clipMode = mode;
            p.ceilingDb = -12;
            engine.process(audio, 2, 4096, p);
        }
        allocation_probe::enabled = false;
        check(allocation_probe::count == 0,
              "oversampled callback and mode changes perform no C++ new/new[]");
    }
    {
        aurum::QualityBank<double> engine;
        aurum::Parameters p;
        p.clipMode = 1;
        p.goldDb = 2;
        engine.prepare(48000, 2, p);
        const int latency = engine.latencySamples();
        std::array<double, 257> l{}, r{};
        double *audio[]{l.data(), r.data()};
        bool finite = true;
        double jump = 0, previous = 0;
        allocation_probe::count = 0;
        allocation_probe::enabled = true;
        for (int block = 0; block < 300; ++block)
        {
            if (block % 50 == 0)
            {
                p.quality = (block / 50) % 3;
                p.filterPhase = (block / 150) % 2;
            }
            for (int i = 0; i < 257; ++i)
            {
                l[size_t(i)] = .2;
                r[size_t(i)] = -.2;
            }
            engine.process(audio, 2, 257, p);
            for (double y : l)
            {
                finite = finite && std::isfinite(y);
                if (block > 2)
                    jump = std::max(jump, std::abs(y - previous));
                previous = y;
            }
        }
        allocation_probe::enabled = false;
        check(finite && engine.latencySamples() == latency,
              "all quality/filter changes preserve fixed PDC and finite output");
        check(allocation_probe::count == 0,
              "quality warmup and crossfades allocate no C++ new/new[]");
        check(jump < .02, "quality switching does not introduce a DC discontinuity above 0.02");
    }
    const auto neutral = master->parameters.snapshot();
    check(neutral.inputDb == 0 && neutral.outputDb == 0 && neutral.wetDb == 0 && neutral.blend == 1,
          "neutral APVTS controls map exactly to the declared parameter grid");
    set(*master, "input", 5.25f);
    set(*master, "blend", 34);
    set(*master, "clipMode", 2);
    set(*master, "ceiling", -9.5f);
    master->editorWidth = 1500;
    juce::MemoryBlock state;
    master->getStateInformation(state);
    restored->setStateInformation(state.getData(), int(state.getSize()));
    check(std::abs(restored->parameters.snapshot().inputDb - 5.25) < .001 &&
              std::abs(restored->parameters.snapshot().blend - .34) < .001,
          "APVTS state round trip");
    check(restored->parameters.snapshot().clipMode == 2 &&
              restored->parameters.snapshot().ceilingDb == -9.5,
          "clipping state round trip");
    check(restored->editorWidth == 1500, "editor width recall");
    track->setStateInformation(state.getData(), int(state.getSize()));
    check(track->parameters.snapshot().inputDb == 0, "cross-variant state rejected");
    auto corrupt = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    corrupt->setAttribute("schema", 999);
    juce::MemoryBlock invalid;
    juce::AudioProcessor::copyXmlToBinary(*corrupt, invalid);
    restored->setStateInformation(invalid.getData(), int(invalid.getSize()));
    check(std::abs(restored->parameters.snapshot().inputDb - 5.25) < .001,
          "future schema leaves state intact");
    restored->setStateInformation("broken", 6);
    check(restored->editorWidth == 1500, "malformed state leaves size intact");
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    for (auto *child : xml->getChildIterator())
        if (child->getStringAttribute("id") == "input")
            child->setAttribute("value", "nan");
    juce::AudioProcessor::copyXmlToBinary(*xml, invalid);
    restored->setStateInformation(invalid.getData(), int(invalid.getSize()));
    check(std::abs(restored->parameters.snapshot().inputDb - 5.25) < .001,
          "nonfinite state rejected transactionally");
    // Phase 1 sessions migrate explicitly to Off, even over an engaged clipper.
    auto legacy = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    legacy->setAttribute("schema", 1);
    for (auto *child = legacy->getFirstChildElement(); child != nullptr;)
    {
        auto *next = child->getNextElement();
        if (child->getStringAttribute("id") == "clipMode" ||
            child->getStringAttribute("id") == "ceiling")
            legacy->removeChildElement(child, true);
        child = next;
    }
    juce::AudioProcessor::copyXmlToBinary(*legacy, invalid);
    restored->setStateInformation(invalid.getData(), int(invalid.getSize()));
    check(restored->parameters.snapshot().clipMode == 0 &&
              restored->parameters.snapshot().ceilingDb == 0,
          "schema 1 migrates to clipping Off");
    check(master->supportsDoublePrecisionProcessing(), "double processing supported");
    for (int channels : {1, 2, 4})
    {
        auto layout = master->getBusesLayout();
        auto channelSet = channels == 1   ? juce::AudioChannelSet::mono()
                          : channels == 2 ? juce::AudioChannelSet::stereo()
                                          : juce::AudioChannelSet::discreteChannels(channels);
        layout.inputBuses.set(0, channelSet);
        layout.outputBuses.set(0, channelSet);
        check(master->isBusesLayoutSupported(layout) == (channels <= 2),
              "matched mono/stereo layouts only");
    }
    // Explicit processor integration, not only direct engine tests.
    set(*master, "input", 0);
    set(*master, "blend", 100);
    set(*master, "clipMode", 0);
    master->prepareToPlay(48000, 17);
    const int latency = master->getLatencySamples();
    check(latency > 0 && latency < 257, "host reports oversampler integer latency");
    juce::AudioBuffer<float> b(2, 257);
    juce::MidiBuffer midi;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample(c, i, c == 0 ? .2f : -.3f);
    master->processBlock(b, midi);
    check(std::abs(b.getSample(0, latency) - .2f) < 1e-7 &&
              std::abs(b.getSample(1, latency) + .3f) < 1e-7,
          "variable block host routing");
    juce::AudioBuffer<double> d(2, 257);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < d.getNumSamples(); ++i)
            d.setSample(c, i, .123456789012);
    master->processBlock(d, midi);
    check(std::abs(d.getSample(0, latency) - .123456789012) < 1e-14,
          "64-bit host processing precision");
    set(*restored, "input", 1);
    restored->presets.selectAB(1);
    set(*restored, "input", 4);
    restored->presets.selectAB(0);
    check(restored->parameters.snapshot().inputDb == 1,
          "A/B starts tracking then preserves independent A");
    restored->presets.selectAB(1);
    check(restored->parameters.snapshot().inputDb == 4, "B restores independent edits");
    restored->presets.copyAB(1, 0);
    restored->presets.selectAB(0);
    check(restored->parameters.snapshot().inputDb == 4, "copy B to A restores exact parameters");
    juce::MemoryBlock abState;
    restored->getStateInformation(abState);
    restored->setStateInformation(abState.getData(), int(abState.getSize()));
    restored->presets.selectAB(1);
    check(restored->parameters.snapshot().inputDb == 4,
          "both A/B states survive host state round trip");
    check(track->parameters.state.getParameter("alchemy") == nullptr &&
              track->parameters.state.getParameter("boxTone") == nullptr,
          "Track does not expose Master-only DSP parameters");
    restored->presets.factory(1);
    check(restored->parameters.snapshot().goldDb == .8 &&
              restored->parameters.snapshot().clipMode == 3,
          "factory preset sets real processing parameters");
    check(restored->presets.saveUser("User Test"), "user preset saves atomically");
    auto userPresets = restored->presets.userPresets();
    check(userPresets.size() == 1, "user preset enumeration");
    if (userPresets.size() == 1)
    {
        set(*restored, "input", 7);
        check(restored->presets.loadUser(userPresets[0]) &&
                  restored->parameters.snapshot().inputDb == 0,
              "user preset restores exact parameters");
        check(restored->presets.renameUser(userPresets[0], "Renamed"), "user preset rename");
        userPresets = restored->presets.userPresets();
        check(restored->presets.removeUser(userPresets[0]), "user preset delete");
    }
    auto editor = std::unique_ptr<juce::AudioProcessorEditor>(restored->createEditor());
    check(editor->getWidth() == 1500 && editor->getHeight() == 750,
          "saved editor dimensions respected");
    editor->setSize(960, 480);
    check(restored->editorWidth == 960 && editor->isResizable(), "host resize saved");
    editor.reset();
    editor.reset(restored->createEditor());
    check(editor->getWidth() == 960, "editor reopen retains size");
    editor.reset();
    if (argc == 3 && juce::String(argv[1]) == "--render")
    {
        juce::File directory(argv[2]);
        directory.createDirectory();
        for (auto *processor : {master.get(), track.get()})
        {
            set(*processor, "clipMode", 2);
            set(*processor, "ceiling", -12);
            set(*processor, "goldMode", 1);
            set(*processor, "goldClassic", 3);
            if (!processor->isCompact())
            {
                set(*processor, "alchemy", 1.5f);
                set(*processor, "boxTone", 2);
            }
            processor->prepareToPlay(48000, 256);
            juce::AudioBuffer<float> audio(2, 256);
            for (int block = 0; block < 800; ++block)
            {
                for (int i = 0; i < 256; ++i)
                {
                    double t = (block * 256 + i) / 48000.0;
                    audio.setSample(
                        0, i, float(.45 * std::sin(juce::MathConstants<double>::twoPi * 440 * t)));
                    audio.setSample(
                        1, i, float(.35 * std::sin(juce::MathConstants<double>::twoPi * 661 * t)));
                }
                processor->processBlock(audio, midi);
            }
            editor.reset(processor->createEditor());
            const int base = processor->isCompact() ? 520 : 1200;
            for (int percent : {80, 100, 175})
            {
                editor->setSize(juce::roundToInt(base * percent / 100.0),
                                juce::roundToInt(600 * percent / 100.0));
                auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1);
                juce::FileOutputStream stream(
                    directory.getChildFile((processor->isCompact() ? "Track-" : "Master-") +
                                           juce::String(percent) + ".png"));
                stream.setPosition(0);
                stream.truncate();
                juce::PNGImageFormat png;
                check(png.writeImageToStream(image, stream), "render actual editor snapshot");
            }
            editor.reset();
        }
    }
    if (argc == 3 && juce::String(argv[1]) == "--measure")
        measurements::exportAll(juce::File(argv[2]));
    testPresetDirectory.deleteRecursively();
    std::cout << checks << " host checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
