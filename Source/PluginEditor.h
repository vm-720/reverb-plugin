#pragma once

#include "PluginProcessor.h"

class SimpleReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleReverbEditor (SimpleReverbProcessor&);
    ~SimpleReverbEditor() override = default;

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
    std::array<Knob, 5> knobs;
    juce::ToggleButton freezeButton { "Freeze" };
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbEditor)
};
