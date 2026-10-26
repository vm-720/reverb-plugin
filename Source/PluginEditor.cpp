#include "PluginEditor.h"

namespace
{
const juce::Colour background { 0xff181b21 };
const juce::Colour panel { 0xff22262e };
const juce::Colour accent { 0xff6fb3ff };
const juce::Colour track { 0xff3a404c };
} // namespace

KnobLookAndFeel::KnobLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.7f));
    setColour (juce::ToggleButton::tickColourId, accent);
}

void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = radius - 3.0f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (track);
    g.strokePath (backgroundArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto knobRadius = arcRadius - 8.0f;
    g.setColour (panel.brighter (0.15f));
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    const juce::Point<float> tip (centre.x + (knobRadius - 4.0f) * std::sin (angle),
                                  centre.y - (knobRadius - 4.0f) * std::cos (angle));
    g.setColour (juce::Colours::white);
    g.drawLine ({ centre.getPointOnCircumference (knobRadius * 0.35f, angle), tip }, 2.5f);
}

SimpleReverbEditor::SimpleReverbEditor (SimpleReverbProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    setUpKnob (topKnobs[0], "predelay", "Pre-Delay");
    setUpKnob (topKnobs[1], "size", "Size");
    setUpKnob (topKnobs[2], "decay", "Decay");
    setUpKnob (topKnobs[3], "damping", "Damping");
    setUpKnob (topKnobs[4], "modulation", "Modulation");

    setUpKnob (bottomKnobs[0], "lowcut", "Low Cut");
    setUpKnob (bottomKnobs[1], "highcut", "High Cut");
    setUpKnob (bottomKnobs[2], "width", "Width");
    setUpKnob (bottomKnobs[3], "mix", "Mix");

    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "freeze", freezeButton);

    setSize (600, 400);
}

SimpleReverbEditor::~SimpleReverbEditor()
{
    setLookAndFeel (nullptr);
}

void SimpleReverbEditor::setUpKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramId, knob.slider);
}

void SimpleReverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (36);
    const auto rowHeight = area.getHeight() / 2;
    g.setColour (panel);
    g.fillRoundedRectangle (area.removeFromTop (rowHeight).reduced (0, 4).toFloat(), 8.0f);
    g.fillRoundedRectangle (area.reduced (0, 4).toFloat(), 8.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("Simple Reverb", getLocalBounds().removeFromTop (46), juce::Justification::centred);
}

void SimpleReverbEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (36);
    const auto rowHeight = area.getHeight() / 2;

    auto layoutRow = [] (juce::Rectangle<int> row, auto& knobs, int columns) {
        row = row.reduced (8, 12);
        const auto width = row.getWidth() / columns;
        for (auto& knob : knobs)
        {
            auto column = row.removeFromLeft (width);
            knob.label.setBounds (column.removeFromTop (20));
            knob.slider.setBounds (column.reduced (4, 0));
        }
        return row;
    };

    layoutRow (area.removeFromTop (rowHeight), topKnobs, 5);
    const auto spare = layoutRow (area, bottomKnobs, 5);
    freezeButton.setBounds (spare.withSizeKeepingCentre (90, 28));
}
