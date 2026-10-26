#pragma once

#include "PluginProcessor.h"

class ClassicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClassicLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool isHighlighted, bool isDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isHighlighted, bool isDown) override;
};

// Small left/right triangle used to step through presets.
class ArrowButton : public juce::Button
{
public:
    explicit ArrowButton (bool pointsRight) : juce::Button ({}), right (pointsRight) {}
    void paintButton (juce::Graphics&, bool isHighlighted, bool isDown) override;

private:
    bool right;
};

class SimpleReverbEditor : public juce::AudioProcessorEditor, private juce::Timer
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

    struct Group
    {
        juce::String heading;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    void setUpKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& suffix,
                    bool small = false);
    void layoutSmallGroup (Group&);
    void timerCallback() override;
    void showPresetMenu();
    void stepPreset (int delta);

    SimpleReverbProcessor& processor;
    ClassicLookAndFeel lookAndFeel;

    Knob mix, width, size, decay, preDelay, density, modRate, modDepth, highCut, lowCut, damping, ducking;
    std::array<Group, 6> groups;

    juce::TextButton freezeButton { "FREEZE" }, clearButton { "CLEAR" };
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    juce::TextButton presetName;
    ArrowButton prevPreset { false }, nextPreset { true };
    int shownPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbEditor)
};
