#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleReverbProcessor::SimpleReverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    sizeParam = apvts.getRawParameterValue ("size");
    dampingParam = apvts.getRawParameterValue ("damping");
    widthParam = apvts.getRawParameterValue ("width");
    preDelayParam = apvts.getRawParameterValue ("predelay");
    mixParam = apvts.getRawParameterValue ("mix");
    freezeParam = apvts.getRawParameterValue ("freeze");
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleReverbProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = AudioParameterFloatAttributes().withLabel ("%")
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)); });

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "size", 1 }, "Size",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.6f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "damping", 1 }, "Damping",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.4f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, "Width",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "predelay", 1 }, "Pre-Delay",
                                                       NormalisableRange<float> (0.0f, maxPreDelayMs, 0.1f, 0.5f), 20.0f,
                                                       AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.3f, percent));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "freeze", 1 }, "Freeze", false));

    return layout;
}

bool SimpleReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void SimpleReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                        (juce::uint32) getTotalNumOutputChannels() };

    reverb.prepare (spec);
    reverb.reset();

    preDelay.setMaximumDelayInSamples ((int) std::ceil (maxPreDelayMs * 0.001 * sampleRate) + 1);
    preDelay.prepare (spec);
    preDelay.reset();

    dryWet.prepare (spec);
    dryWet.reset();

    preDelaySamples.reset (sampleRate, 0.05);
    preDelaySamples.setCurrentAndTargetValue (preDelayParam->load() * 0.001f * (float) sampleRate);
}

void SimpleReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = getTotalNumOutputChannels();
    for (auto ch = getTotalNumInputChannels(); ch < numChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    juce::dsp::Reverb::Parameters params;
    params.roomSize = sizeParam->load();
    params.damping = dampingParam->load();
    params.width = widthParam->load();
    params.freezeMode = freezeParam->load() > 0.5f ? 1.0f : 0.0f;
    params.wetLevel = 1.0f; // dry/wet is handled by the mixer so pre-delay only affects the wet path
    params.dryLevel = 0.0f;
    reverb.setParameters (params);

    dryWet.setWetMixProportion (mixParam->load());

    juce::dsp::AudioBlock<float> block (buffer);
    dryWet.pushDrySamples (block);

    preDelaySamples.setTargetValue (preDelayParam->load() * 0.001f * (float) getSampleRate());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        preDelay.setDelay (preDelaySamples.getNextValue());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            preDelay.pushSample (ch, data[i]);
            data[i] = preDelay.popSample (ch);
        }
    }

    reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
    dryWet.mixWetSamples (block);
}

void SimpleReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SimpleReverbProcessor::createEditor()
{
    return new SimpleReverbEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleReverbProcessor();
}
