#pragma once
#include "plugin/PluginProcessor.h"
namespace aurum
{
class CustomLookAndFeel final : public juce::LookAndFeel_V4
{
  public:
    CustomLookAndFeel();
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider &) override;
    void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
    void drawCornerResizer(juce::Graphics &, int, int, bool, bool) override;
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          const juce::Slider::SliderStyle, juce::Slider &) override;
};
class PluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
    PluginProcessor &processor;
    CustomLookAndFeel look;
    juce::TooltipWindow tips{this, 500};
    juce::Component canvas;
    std::array<juce::Slider, 9> sliders;
    std::array<juce::Label, 9> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 9>
        sliderLinks;
    std::array<juce::ToggleButton, 10> toggles;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 10>
        toggleLinks;
    class ScaleHit final : public juce::Component
    {
      public:
        std::function<void()> cycle;
        void mouseDoubleClick(const juce::MouseEvent &) override
        {
            if (cycle)
                cycle();
        }
    };
    std::array<ScaleHit, 2> scaleHits;
    juce::ComboBox scale, clipMode, goldMode, boxTone, preset;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> clipLink, goldModeLink,
        toneLink;
    juce::TextButton reset{"RESET"}, settings{"SETTINGS"}, save{"SAVE"}, previous{"<"}, next{">"},
        a{"A"}, b{"B"}, copy{"A > B"};
    std::array<Envelope, 400> history{};
    unsigned head = 0;
    std::array<float, 2> inputHold{}, outputHold{}, inputNow{}, outputNow{};
    bool overloaded = false, tooltips = true;
    int baseWidth, baseHeight = 600, meterScale = 0, lastGoldMode = -1;
    Telemetry::Analysis analysis{NAN, NAN, 0, NAN};
    juce::Array<juce::File> userFiles;
    void timerCallback() override;
    void drawWaveform(juce::Graphics &, juce::Rectangle<float>);
    void drawMeter(juce::Graphics &, juce::Rectangle<float>, bool);
    void showSettings();
    void refreshPresets();
    void namePreset(bool rename);
    void setParameter(const char *, float);
    void editNumber(const char *, const juce::String &);
    juce::File preferencesFile() const;
    void loadPreferences();
    void savePreferences();

  public:
    explicit PluginEditor(PluginProcessor &);
    ~PluginEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent &) override;
};
} // namespace aurum
