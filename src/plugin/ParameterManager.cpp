#include "ParameterManager.h"
#include <locale>
#include <sstream>
namespace aurum
{
juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::layout(bool compact)
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto number =
        [&](const char *id, const char *name, float lo, float hi, float def, const char *unit)
    {
        p.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{lo, hi, .01f}, def,
            juce::AudioParameterFloatAttributes().withLabel(unit).withStringFromValueFunction(
                [](float x, int) { return juce::String(std::abs(x) < .005f ? 0.f : x, 1); })));
    };
    auto boolean = [&](const char *id, const char *name, bool value)
    { p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, value)); };
    auto choice = [&](const char *id, const char *name, juce::StringArray options)
    {
        p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, options,
                                                           0));
    };
    number("input", "Input Trim", -12, 12, 0, "dB");
    number("output", "Output Trim", -12, 12, 0, "dB");
    number("blend", "Linked Wet Blend", 0, 100, 100, "%");
    number("wet", "Independent Wet Level", -60, 6, 0, "dB");
    number("dry", "Independent Dry Level", -60, 6, -60, "dB");
    boolean("linked", "Link Wet/Dry", true);
    boolean("wetMute", "Mute Wet", false);
    boolean("dryMute", "Mute Dry", false);
    boolean("delta", "Delta Monitor", false);
    boolean("bypass", "Bypass", false);
    number("ceiling", "Clip Ceiling", -36, 0, 0, "dBFS");
    choice("clipMode", "Clip Mode", {"Off", "Hard", "Classic", "Modern"});
    choice("goldMode", "Gold Curve", {"Modern", "Classic"});
    number("goldModern", "Modern Gold", 0, 2.5, 0, "dB");
    number("goldClassic", "Classic Gold", 0, 6, 0, "dB");
    boolean("goldUnity", "Gold Unity", false);
    if (!compact)
    {
        number("alchemy", "Alchemy", 0, 2, 0, "dB");
        choice("boxTone", "Box Tone", {"Flat", "Classic", "Modern"});
    }
    choice("quality", "Realtime Quality", {"High", "Pristine", "Extra Pristine"});
    choice("offlineQuality", "Offline Quality", {"High", "Pristine", "Extra Pristine"});
    choice("filterPhase", "Oversampling Phase", {"Linear FIR", "Minimum IIR"});
    boolean("clipFilter", "Clip Filter", false);
    number("clipFilterHz", "Clip Filter Cutoff", 4000, 22000, 20000, "Hz");
    boolean("trimLinked", "Link Trims", false);
    number("trimOffset", "Linked Trim Offset", -24, 24, 0, "dB");
    boolean("waveform", "Waveform Display", true);
    boolean("peakGuard", "Wet Sample Peak Guard", true);
    choice("meterScale", "Meter Scale", {"+6 to -36 dBFS", "+4 to -24 dBFS", "+2 to -12 dBFS"});
    return p;
}
ParameterManager::ParameterManager(juce::AudioProcessor &owner, bool isCompact)
    : compact(isCompact), state(owner, nullptr, "AurumState", layout(isCompact))
{
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = state.getRawParameterValue(ids[i]);
}
Parameters ParameterManager::snapshot() const noexcept
{
    auto v = [&](size_t i, double fallback = 0.)
    { return values[i] ? std::round(double(values[i]->load()) * 100) / 100 : fallback; };
    Parameters p{v(0),      v(1),      v(2) / 100, v(3),      v(4),  v(5) > .5,
                 v(6) > .5, v(7) > .5, v(8) > .5,  v(9) > .5, v(10), int(v(11))};
    p.goldMode = int(v(12));
    p.goldDb = v(p.goldMode == 0 ? 13 : 14);
    p.goldUnity = v(15) > .5;
    p.alchemyDb = v(16);
    p.boxTone = int(v(17));
    p.quality = int(v(18));
    p.offlineQuality = int(v(19));
    p.filterPhase = int(v(20));
    p.clipFilter = v(21) > .5;
    p.clipFilterHz = v(22, 20000);
    p.trimLinked = v(23) > .5;
    p.trimOffset = v(24);
    p.waveform = v(25) > .5;
    p.peakGuard = v(26) > .5;
    p.meterScale = int(v(27));
    return validated(p);
}
void ParameterManager::save(juce::MemoryBlock &bytes, bool isCompact, int width)
{
    auto tree = state.copyState();
    tree.setProperty("schema", 3, nullptr);
    tree.setProperty("variant", isCompact ? "track" : "master", nullptr);
    tree.setProperty("editorWidth", width, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), bytes);
}
bool ParameterManager::restore(const void *bytes, int size, bool isCompact, int &width, bool apply)
{
    if (size <= 0 || size > 1024 * 1024)
        return false;
    auto xml = juce::AudioProcessor::getXmlFromBinary(bytes, size);
    if (!xml || !xml->hasTagName("AurumState"))
        return false;
    auto tree = juce::ValueTree::fromXml(*xml);
    int schema = int(tree.getProperty("schema", 0));
    if (schema < 1 || schema > 3 ||
        tree.getProperty("variant").toString() != (isCompact ? "track" : "master"))
        return false;
    std::array<float, std::size(ids)> restored{};
    std::array<bool, std::size(ids)> found{};
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (!values[i])
        {
            found[i] = true;
            continue;
        }
        auto *parameter = state.getParameter(ids[i]);
        restored[i] = parameter->convertFrom0to1(parameter->getDefaultValue());
        if ((schema == 1 && i >= 10) || (schema == 2 && i >= 12))
            found[i] = true;
    }
    for (auto child : tree)
    {
        const auto id = child.getProperty("id").toString();
        for (size_t i = 0; i < values.size(); ++i)
            if (values[i] && id == ids[i])
            {
                // Legacy snapshots may include newer entries (e.g. manual schema edits); consume
                // only the schema's contract.
                if ((schema == 1 && i >= 10) || (schema == 2 && i >= 12))
                    continue;
                if (found[i] || !child.hasProperty("value"))
                    return false;
                const auto text = child.getProperty("value").toString().toStdString();
                float value = 0;
                std::istringstream parser(text);
                parser.imbue(std::locale::classic());
                parser >> std::noskipws >> value;
                if (!parser || !parser.eof() || !std::isfinite(value))
                    return false;
                auto range = state.getParameter(ids[i])->getNormalisableRange();
                restored[i] = juce::jlimit(range.start, range.end, value);
                found[i] = true;
            }
    }
    for (bool valid : found)
        if (!valid)
            return false;
    if (!apply)
        return true;
    for (size_t i = 0; i < values.size(); ++i)
        if (values[i])
        {
            auto *parameter = state.getParameter(ids[i]);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(restored[i]));
        }
    const int base = isCompact ? 520 : 1200;
    width =
        juce::jlimit(int(base * .8), int(base * 1.75), int(tree.getProperty("editorWidth", base)));
    return true;
}
} // namespace aurum
