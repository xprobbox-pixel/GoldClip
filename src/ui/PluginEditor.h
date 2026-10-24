#pragma once
#include "plugin/PluginProcessor.h"
namespace aurum {
class CustomLookAndFeel final:public juce::LookAndFeel_V4 {
public:
 CustomLookAndFeel();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool)override;
 void drawCornerResizer(juce::Graphics&,int,int,bool,bool)override;
};
class PluginEditor final:public juce::AudioProcessorEditor,private juce::Timer {
 PluginProcessor& processor;CustomLookAndFeel look;juce::TooltipWindow tips{this,500};
 juce::Component canvas;
 std::array<juce::Slider,5> sliders;
 std::array<juce::Label,5> labels;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,5> sliderLinks;
 std::array<juce::ToggleButton,5> toggles;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,5> toggleLinks;
 juce::ComboBox scale;juce::TextButton reset{"RESET PEAKS"};
 std::array<Envelope,400> history{};unsigned head=0;
 Telemetry::Peaks peaks{};float inputHold=0,outputHold=0;bool overloaded=false;
 int baseWidth,baseHeight=600;
 void timerCallback()override;
 void drawWaveform(juce::Graphics&,juce::Rectangle<float>);
public:
 explicit PluginEditor(PluginProcessor&);
 ~PluginEditor()override;
 void paint(juce::Graphics&)override;
 void resized()override;
};
}
