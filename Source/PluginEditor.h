#pragma once

#include "PluginProcessor.h"

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};

class SimpleReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleReverbEditor (SimpleReverbProcessor&);
    ~SimpleReverbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setUpKnob (Knob&, const juce::String& paramId, const juce::String& name);

    SimpleReverbProcessor& processor;
    KnobLookAndFeel lookAndFeel;

    // Top row: space and time. Bottom row: tone and output.
    std::array<Knob, 5> topKnobs;
    std::array<Knob, 4> bottomKnobs;
    juce::ToggleButton freezeButton { "Freeze" };
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbEditor)
};
