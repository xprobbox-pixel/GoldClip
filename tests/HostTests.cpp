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
    juce::ScopedJuceInitialiser_GUI gui;
    // Processors must outlive all editors and parameter attachments.
    auto master = std::make_unique<aurum::PluginProcessor>(false),
         restored = std::make_unique<aurum::PluginProcessor>(false),
         track = std::make_unique<aurum::PluginProcessor>(true);
    set(*master, "input", 5.25f);
    set(*master, "blend", 34);
    master->editorWidth = 1500;
    juce::MemoryBlock state;
    master->getStateInformation(state);
    restored->setStateInformation(state.getData(), int(state.getSize()));
    check(std::abs(restored->parameters.snapshot().inputDb - 5.25) < .001 &&
              std::abs(restored->parameters.snapshot().blend - .34) < .001,
          "APVTS state round trip");
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
    check(master->supportsDoublePrecisionProcessing() && master->getLatencySamples() == 0,
          "double processing and zero reported latency");
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
    master->prepareToPlay(48000, 17);
    juce::AudioBuffer<float> b(2, 257);
    juce::MidiBuffer midi;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample(c, i, c == 0 ? .2f : -.3f);
    master->processBlock(b, midi);
    check(std::abs(b.getSample(0, 0) - .2f) < 1e-7 && std::abs(b.getSample(1, 0) + .3f) < 1e-7,
          "variable block host routing");
    juce::AudioBuffer<double> d(2, 17);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 17; ++i)
            d.setSample(c, i, .123456789012);
    master->processBlock(d, midi);
    check(std::abs(d.getSample(0, 0) - .123456789012) < 1e-14, "64-bit host processing precision");
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
            processor->prepareToPlay(48000, 256);
            juce::AudioBuffer<float> audio(2, 256);
            for (int block = 0; block < 120; ++block)
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
    std::cout << checks << " host checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
