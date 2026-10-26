#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Presets.h"
#include "ReverbEngine.h"

class SimpleReverbProcessor : public juce::AudioProcessor
{
public:
    SimpleReverbProcessor();
    ~SimpleReverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return (int) getFactoryPresets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Called from the editor; the audio thread clears the tail on its next block.
    void requestClear() { clearRequested = true; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr float maxPreDelayMs = 500.0f;

    reverb::Engine engine;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelay;
    juce::dsp::DryWetMixer<float> dryWet;
    juce::SmoothedValue<float> preDelaySamples;
    juce::AudioBuffer<float> monoScratch;
    std::vector<float> duckGain;
    float duckEnvelope = 0.0f, duckAttack = 0.0f, duckRelease = 0.0f;
    std::atomic<bool> clearRequested { false };
    int currentPreset = 0;

    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* dampingParam = nullptr;
    std::atomic<float>* lowCutParam = nullptr;
    std::atomic<float>* highCutParam = nullptr;
    std::atomic<float>* modParam = nullptr;
    std::atomic<float>* modRateParam = nullptr;
    std::atomic<float>* densityParam = nullptr;
    std::atomic<float>* duckingParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* preDelayParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleReverbProcessor)
};
