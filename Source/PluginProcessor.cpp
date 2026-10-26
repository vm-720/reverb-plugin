#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleReverbProcessor::SimpleReverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    sizeParam = apvts.getRawParameterValue ("size");
    decayParam = apvts.getRawParameterValue ("decay");
    dampingParam = apvts.getRawParameterValue ("damping");
    lowCutParam = apvts.getRawParameterValue ("lowcut");
    highCutParam = apvts.getRawParameterValue ("highcut");
    modParam = apvts.getRawParameterValue ("modulation");
    modRateParam = apvts.getRawParameterValue ("modrate");
    densityParam = apvts.getRawParameterValue ("density");
    duckingParam = apvts.getRawParameterValue ("ducking");
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

    auto hertz = AudioParameterFloatAttributes().withLabel ("Hz")
                     .withStringFromValueFunction ([] (float v, int) {
                         return v >= 1000.0f ? String (v / 1000.0f, 1) + "k" : String (roundToInt (v));
                     });

    auto seconds = AudioParameterFloatAttributes().withLabel ("s")
                       .withStringFromValueFunction ([] (float v, int) { return String (v, v < 10.0f ? 2 : 1); });

    auto logRange = [] (float min, float max) {
        return NormalisableRange<float> (min, max,
                                         [] (float lo, float hi, float t) { return lo * std::pow (hi / lo, t); },
                                         [] (float lo, float hi, float v) { return std::log (v / lo) / std::log (hi / lo); });
    };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "predelay", 1 }, "Pre-Delay",
                                                       NormalisableRange<float> (0.0f, maxPreDelayMs, 0.1f, 0.5f), 20.0f,
                                                       AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "size", 1 }, "Size",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.5f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay",
                                                       logRange (0.2f, 20.0f), 2.5f, seconds));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "damping", 1 }, "Damping",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.4f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "modulation", 1 }, "Modulation",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.3f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "modrate", 1 }, "Mod Rate",
                                                       logRange (0.05f, 5.0f), 0.5f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")
                                                           .withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "density", 1 }, "Density",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.8f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "ducking", 1 }, "Ducking",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "lowcut", 1 }, "Low Cut",
                                                       logRange (20.0f, 1000.0f), 100.0f, hertz));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "highcut", 1 }, "High Cut",
                                                       logRange (1000.0f, 20000.0f), 12000.0f, hertz));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, "Width",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, percent));
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

    engine.prepare (sampleRate);

    preDelay.setMaximumDelayInSamples ((int) std::ceil (maxPreDelayMs * 0.001 * sampleRate) + 1);
    preDelay.prepare (spec);
    preDelay.reset();

    dryWet.prepare (spec);
    dryWet.reset();

    monoScratch.setSize (1, samplesPerBlock);
    duckGain.assign ((size_t) samplesPerBlock, 1.0f);
    duckEnvelope = 0.0f;
    duckAttack = 1.0f - (float) std::exp (-1.0 / (0.01 * sampleRate));
    duckRelease = 1.0f - (float) std::exp (-1.0 / (0.25 * sampleRate));

    preDelaySamples.reset (sampleRate, 0.05);
    preDelaySamples.setCurrentAndTargetValue (preDelayParam->load() * 0.001f * (float) sampleRate);
}

void SimpleReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    for (auto ch = getTotalNumInputChannels(); ch < numChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    reverb::Parameters params;
    params.size = sizeParam->load();
    params.decaySeconds = decayParam->load();
    params.damping = dampingParam->load();
    params.lowCutHz = lowCutParam->load();
    params.highCutHz = highCutParam->load();
    params.modulation = modParam->load();
    params.modRateHz = modRateParam->load();
    params.density = densityParam->load();
    params.width = widthParam->load();
    params.freeze = freezeParam->load() > 0.5f;
    engine.setParameters (params);

    if (clearRequested.exchange (false))
    {
        engine.reset();
        preDelay.reset();
    }

    dryWet.setWetMixProportion (mixParam->load());

    juce::dsp::AudioBlock<float> block (buffer);
    dryWet.pushDrySamples (block);

    // Ducking follows the dry input level and pulls the wet signal down while you play.
    const auto ducking = duckingParam->load();
    if (duckGain.size() < (size_t) numSamples)
        duckGain.resize ((size_t) numSamples);

    preDelaySamples.setTargetValue (preDelayParam->load() * 0.001f * (float) getSampleRate());
    for (int i = 0; i < numSamples; ++i)
    {
        preDelay.setDelay (preDelaySamples.getNextValue());
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            peak = std::fmax (peak, std::fabs (data[i]));
            preDelay.pushSample (ch, data[i]);
            data[i] = preDelay.popSample (ch);
        }
        duckEnvelope += (peak > duckEnvelope ? duckAttack : duckRelease) * (peak - duckEnvelope);
        duckGain[(size_t) i] = 1.0f / (1.0f + ducking * duckEnvelope * 30.0f);
    }

    if (numChannels >= 2)
    {
        engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
    }
    else
    {
        // Mono: run the engine in stereo on a copy and fold the result back down.
        monoScratch.setSize (1, numSamples, false, false, true);
        auto* mono = buffer.getWritePointer (0);
        auto* copy = monoScratch.getWritePointer (0);
        std::copy (mono, mono + numSamples, copy);
        engine.process (mono, copy, numSamples);
        for (int i = 0; i < numSamples; ++i)
            mono[i] = 0.5f * (mono[i] + copy[i]);
    }

    if (ducking > 0.0f)
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::multiply (buffer.getWritePointer (ch), duckGain.data(), numSamples);

    dryWet.mixWetSamples (block);
}

void SimpleReverbProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    currentPreset = index;
    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            auto value = ranged->getDefaultValue();
            for (const auto& [id, plain] : presets[(size_t) index].values)
                if (ranged->paramID == id)
                    value = ranged->convertTo0to1 (plain);

            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (value);
            ranged->endChangeGesture();
        }
}

const juce::String SimpleReverbProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    return index >= 0 && index < (int) presets.size() ? presets[(size_t) index].name : "";
}

void SimpleReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* SimpleReverbProcessor::createEditor()
{
    return new SimpleReverbEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleReverbProcessor();
}
