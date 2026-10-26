#pragma once
#include "ParameterManager.h"
#include "dsp/OversampledEngine.h"
namespace aurum
{
class PluginProcessor final : public juce::AudioProcessor
{
    bool compact;
    OversampledEngine<float> floatEngine;
    OversampledEngine<double> doubleEngine;
    template <class T> void process(juce::AudioBuffer<T> &b, bool bypassed)
    {
        juce::ScopedNoDenormals noDenormals;
        auto p = parameters.snapshot();
        auto &engine = [&]() -> auto &
        {
            if constexpr (std::is_same_v<T, float>)
                return floatEngine;
            else
                return doubleEngine;
        }();
        engine.process(b.getArrayOfWritePointers(), unsigned(b.getNumChannels()), b.getNumSamples(),
                       p, &telemetry, bypassed);
    }

  public:
    ParameterManager parameters;
    Telemetry telemetry;
    std::atomic<int> editorWidth;
    explicit PluginProcessor(bool isCompact = AURUM_COMPACT != 0);
    bool isCompact() const noexcept
    {
        return compact;
    }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    void processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &) override
    {
        process(b, false);
    }
    void processBlock(juce::AudioBuffer<double> &b, juce::MidiBuffer &) override
    {
        process(b, false);
    }
    void processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &) override
    {
        process(b, true);
    }
    void processBlockBypassed(juce::AudioBuffer<double> &b, juce::MidiBuffer &) override
    {
        process(b, true);
    }
    bool supportsDoublePrecisionProcessing() const override
    {
        return true;
    }
    juce::AudioProcessorParameter *getBypassParameter() const override
    {
        return parameters.state.getParameter("bypass");
    }
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }
    const juce::String getName() const override
    {
        return compact ? "AURUM CLIP Track" : "AURUM CLIP Master";
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override
    {
        return false;
    }
    bool isMidiEffect() const override
    {
        return false;
    }
    double getTailLengthSeconds() const override
    {
        return 0;
    }
    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return "Clip Off Default";
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &b) override
    {
        parameters.save(b, compact, editorWidth.load());
    }
    void setStateInformation(const void *bytes, int size) override
    {
        int width = editorWidth.load();
        if (parameters.restore(bytes, size, compact, width))
            editorWidth = width;
    }
};
} // namespace aurum
