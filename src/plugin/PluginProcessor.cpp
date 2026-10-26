#include "PluginProcessor.h"
#include "ui/PluginEditor.h"
namespace aurum
{
PluginProcessor::PluginProcessor(bool isCompact)
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      compact(isCompact), parameters(*this), editorWidth(isCompact ? 520 : 1200)
{
    setLatencySamples(0);
}
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout &layout) const
{
    const auto in = layout.getMainInputChannelSet(), out = layout.getMainOutputChannelSet();
    return in == out &&
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}
void PluginProcessor::prepareToPlay(double rate, int)
{
    const auto p = parameters.snapshot();
    floatEngine.prepare(rate, unsigned(getTotalNumInputChannels()), p);
    doubleEngine.prepare(rate, unsigned(getTotalNumInputChannels()), p);
    setLatencySamples(floatEngine.latencySamples());
}
juce::AudioProcessorEditor *PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}
} // namespace aurum
