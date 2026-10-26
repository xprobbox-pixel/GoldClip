#pragma once
#include "dsp/Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>
namespace aurum
{
class ParameterManager
{
    std::array<std::atomic<float> *, 12> values{};

  public:
    juce::AudioProcessorValueTreeState state;
    explicit ParameterManager(juce::AudioProcessor &);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    Parameters snapshot() const noexcept;
    void save(juce::MemoryBlock &, bool compact, int width);
    bool restore(const void *, int, bool compact, int &width);
};
} // namespace aurum
