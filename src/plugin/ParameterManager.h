#pragma once
#include "dsp/Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>
namespace aurum
{
class ParameterManager
{
    bool compact = false;
    static constexpr const char *ids[]{
        "input",      "output",         "blend",       "wet",        "dry",          "linked",
        "wetMute",    "dryMute",        "delta",       "bypass",     "ceiling",      "clipMode",
        "goldMode",   "goldModern",     "goldClassic", "goldUnity",  "alchemy",      "boxTone",
        "quality",    "offlineQuality", "filterPhase", "clipFilter", "clipFilterHz", "trimLinked",
        "trimOffset", "waveform",       "peakGuard",   "meterScale"};
    std::array<std::atomic<float> *, std::size(ids)> values{};

  public:
    juce::AudioProcessorValueTreeState state;
    explicit ParameterManager(juce::AudioProcessor &, bool isCompact = false);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout(bool compact = false);
    Parameters snapshot() const noexcept;
    void save(juce::MemoryBlock &, bool, int);
    bool restore(const void *, int, bool, int &, bool apply = true);
};
} // namespace aurum
