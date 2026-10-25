#include "PluginEditor.h"

SimpleReverbEditor::SimpleReverbEditor (SimpleReverbProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setUpKnob (knobs[0], "size", "Size");
    setUpKnob (knobs[1], "damping", "Damping");
    setUpKnob (knobs[2], "width", "Width");
    setUpKnob (knobs[3], "predelay", "Pre-Delay");
    setUpKnob (knobs[4], "mix", "Mix");

    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "freeze", freezeButton);

    setSize (560, 220);
}

void SimpleReverbEditor::setUpKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff6fb3ff));
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramId, knob.slider);
}

void SimpleReverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1f26));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("Simple Reverb", getLocalBounds().removeFromTop (40), juce::Justification::centred);
}

void SimpleReverbEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (30);
    freezeButton.setBounds (area.removeFromBottom (24).withSizeKeepingCentre (90, 24));

    const auto knobWidth = area.getWidth() / (int) knobs.size();
    for (auto& knob : knobs)
    {
        auto column = area.removeFromLeft (knobWidth);
        knob.label.setBounds (column.removeFromTop (20));
        knob.slider.setBounds (column.reduced (4));
    }
}
