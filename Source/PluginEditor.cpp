#include "PluginEditor.h"

namespace
{
const juce::Colour background { 0xff070d22 };
const juce::Colour valueText { 0xffaec8ff };
const juce::Colour titleAccent { 0xff7fa6ff };
const juce::Colour panelEdge { 0x40ffffff };

// Panels shade from bright to deep blue, left to right.
const std::array<juce::Colour, 6> panelColours { juce::Colour (0xff4169e1), juce::Colour (0xff3a5ed0),
                                                 juce::Colour (0xff3453bc), juce::Colour (0xff2f49a8),
                                                 juce::Colour (0xff2a3f94), juce::Colour (0xff253682) };

constexpr int editorWidth = 900, editorHeight = 420;
constexpr int margin = 18, gap = 10, headerHeight = 78, panelHeight = 250;
constexpr int smallGroupWidth = 104, bigGroupWidth = 290;

juce::Font boldFont (float height) { return juce::FontOptions (height, juce::Font::bold); }
} // namespace

//==============================================================================
ClassicLookAndFeel::ClassicLookAndFeel()
{
#if JUCE_WINDOWS
    setDefaultSansSerifTypefaceName ("Segoe UI");
#elif JUCE_MAC
    setDefaultSansSerifTypefaceName ("Avenir Next");
#endif

    setColour (juce::Slider::textBoxTextColourId, valueText);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, valueText.withAlpha (0.3f));
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.3f));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141a3a));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, titleAccent);
}

void ClassicLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 2.0f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (centre.x - radius, centre.y - radius + 2.0f, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colours::black);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Thick white pointer bar from near the centre to the rim.
    const auto barWidth = juce::jmax (4.0f, radius * 0.14f);
    juce::Path pointer;
    pointer.addRoundedRectangle (-barWidth / 2.0f, -radius * 0.92f, barWidth, radius * 0.6f, barWidth * 0.2f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

juce::Label* ClassicLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (boldFont (15.0f));
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

void ClassicLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                               bool isHighlighted, bool isDown)
{
    // Pill buttons (Freeze/Clear). The preset name button is drawn as plain text.
    if (button.getProperties()["plain"])
        return;

    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto fill = button.getToggleState() ? titleAccent : juce::Colours::white;
    if (isDown)
        fill = fill.darker (0.2f);
    else if (isHighlighted)
        fill = fill.brighter (0.1f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, bounds.getHeight() / 2.0f);
}

void ClassicLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isHighlighted, bool)
{
    const bool plain = button.getProperties()["plain"];
    g.setFont (boldFont (plain ? 22.0f : 16.0f));
    g.setColour (plain ? (isHighlighted ? valueText : titleAccent)
                       : (button.getToggleState() ? juce::Colours::white : background));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                plain ? juce::Justification::centredLeft : juce::Justification::centred);
}

//==============================================================================
void ArrowButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    const auto b = getLocalBounds().toFloat().reduced (4.0f);
    juce::Path triangle;
    if (right)
        triangle.addTriangle (b.getX(), b.getY(), b.getRight(), b.getCentreY(), b.getX(), b.getBottom());
    else
        triangle.addTriangle (b.getRight(), b.getY(), b.getX(), b.getCentreY(), b.getRight(), b.getBottom());

    g.setColour (isDown ? valueText : (isHighlighted ? juce::Colours::white : juce::Colours::white.withAlpha (0.9f)));
    g.fillPath (triangle);
}

//==============================================================================
SimpleReverbEditor::SimpleReverbEditor (SimpleReverbProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);

    setUpKnob (mix, "mix", "MIX", " %");
    setUpKnob (width, "width", "WIDTH", " %");
    setUpKnob (size, "size", "SIZE", " %");
    setUpKnob (decay, "decay", "DECAY", " s");
    setUpKnob (preDelay, "predelay", "PRE-DELAY", " ms");
    setUpKnob (density, "density", "DENSITY", " %");
    setUpKnob (modRate, "modrate", "Rate", " Hz", true);
    setUpKnob (modDepth, "modulation", "Depth", " %", true);
    setUpKnob (highCut, "highcut", "High", " Hz", true);
    setUpKnob (lowCut, "lowcut", "Low", " Hz", true);
    setUpKnob (damping, "damping", "DAMPING", " %");
    setUpKnob (ducking, "ducking", "DUCKING", " %");

    groups[0] = { {}, { &mix, &width }, {} };
    groups[1] = { {}, { &size, &decay }, {} };
    groups[2] = { {}, { &preDelay, &density }, {} };
    groups[3] = { "MOD", { &modRate, &modDepth }, {} };
    groups[4] = { "EQ", { &highCut, &lowCut }, {} };
    groups[5] = { {}, { &damping, &ducking }, {} };

    freezeButton.setClickingTogglesState (true);
    addAndMakeVisible (freezeButton);
    freezeAttachment = std::make_unique<ButtonAttachment> (processor.apvts, "freeze", freezeButton);

    clearButton.onClick = [this] { processor.requestClear(); };
    addAndMakeVisible (clearButton);

    presetName.getProperties().set ("plain", true);
    presetName.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetName);

    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);

    setSize (editorWidth, editorHeight);
    timerCallback();
    startTimerHz (10);
}

SimpleReverbEditor::~SimpleReverbEditor()
{
    setLookAndFeel (nullptr);
}

void SimpleReverbEditor::setUpKnob (Knob& knob, const juce::String& paramId, const juce::String& name,
                                    const juce::String& suffix, bool small)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 20);
    knob.slider.setTextValueSuffix (suffix);
    knob.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knob.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setFont (boldFont (small ? 17.0f : 18.0f));
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramId, knob.slider);
}

void SimpleReverbEditor::timerCallback()
{
    const auto current = processor.getCurrentProgram();
    if (current != shownPreset)
    {
        shownPreset = current;
        presetName.setButtonText (processor.getProgramName (current));
    }
}

void SimpleReverbEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        menu.addItem (i + 1, processor.getProgramName (i), true, i == processor.getCurrentProgram());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetName),
                        [this] (int result) {
                            if (result > 0)
                                processor.setCurrentProgram (result - 1);
                            timerCallback();
                        });
}

void SimpleReverbEditor::stepPreset (int delta)
{
    const auto count = processor.getNumPrograms();
    processor.setCurrentProgram ((processor.getCurrentProgram() + delta + count) % count);
    timerCallback();
}

void SimpleReverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    // Title: "Simple" in white, "Reverb" in the accent colour.
    auto titleArea = juce::Rectangle<int> (margin + 4, 6, 500, headerHeight - 10);
    const auto titleFont = boldFont (46.0f);
    const juce::String first ("Simple"), second ("Reverb");
    g.setFont (titleFont);
    g.setColour (juce::Colours::white);
    g.drawText (first, titleArea, juce::Justification::centredLeft);
    g.setColour (titleAccent);
    g.drawText (second, titleArea.withTrimmedLeft (juce::GlyphArrangement::getStringWidthInt (titleFont, first) + 2),
                juce::Justification::centredLeft);

    g.setColour (juce::Colours::white);
    g.setFont (boldFont (14.0f));
    auto info = juce::Rectangle<int> (getWidth() - margin - 260, 18, 260, 18);
    g.drawText ("Version " JucePlugin_VersionString, info, juce::Justification::centredRight);
    g.drawText ("Vincent Marcello", info.translated (0, 18), juce::Justification::centredRight);

    for (size_t i = 0; i < groups.size(); ++i)
    {
        const auto r = groups[i].bounds.toFloat();
        g.setColour (panelColours[i]);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

        if (groups[i].heading.isNotEmpty())
        {
            g.setColour (juce::Colours::white);
            g.setFont (boldFont (19.0f));
            g.drawText (groups[i].heading, groups[i].bounds.withHeight (34).translated (0, 6),
                        juce::Justification::centred);
        }
    }

    g.setColour (juce::Colours::white);
    g.setFont (boldFont (20.0f));
    g.drawText ("PRESET:", juce::Rectangle<int> (margin + 4, editorHeight - 62, 100, 40), juce::Justification::centredLeft);
}

void SimpleReverbEditor::layoutSmallGroup (Group& group)
{
    auto area = group.bounds.reduced (6, 8);
    if (group.heading.isNotEmpty())
        area.removeFromTop (32);

    const auto cellHeight = area.getHeight() / (int) group.knobs.size();
    for (auto* knob : group.knobs)
    {
        auto cell = area.removeFromTop (cellHeight);
        knob->label.setBounds (cell.removeFromTop (24));
        knob->slider.setBounds (cell);
    }
}

void SimpleReverbEditor::resized()
{
    int x = margin;
    const int y = headerHeight + 6;
    for (size_t i = 0; i < groups.size(); ++i)
    {
        const auto w = i == 1 ? bigGroupWidth : smallGroupWidth;
        groups[i].bounds = { x, y, w, panelHeight };
        x += w + gap;

        if (i != 1)
            layoutSmallGroup (groups[i]);
    }

    // Big group: two large knobs side by side with Freeze/Clear underneath.
    auto big = groups[1].bounds.reduced (10, 8);
    auto buttons = big.removeFromBottom (36);
    freezeButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (10, 4));
    clearButton.setBounds (buttons.reduced (10, 4));

    for (auto* knob : groups[1].knobs)
    {
        auto column = big.removeFromLeft (groups[1].bounds.getWidth() / 2 - 10);
        knob->label.setBounds (column.removeFromTop (26));
        knob->slider.setBounds (column);
    }

    auto footer = juce::Rectangle<int> (margin + 110, editorHeight - 62, editorWidth - 2 * margin - 110, 40);
    nextPreset.setBounds (footer.removeFromRight (30).withSizeKeepingCentre (26, 24));
    footer.removeFromRight (6);
    prevPreset.setBounds (footer.removeFromRight (30).withSizeKeepingCentre (26, 24));
    presetName.setBounds (footer.removeFromLeft (360));
}
