#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

// shared DSP
#include "../../../shared/DSP/EnvelopeFollower.h"
#include "../../../shared/DSP/ParameterSmoother.h"
#include "../../../shared/DSP/Meter.h"
#include "../../../shared/DSP/SoftClipper.h"

//==============================================================================
class BoDSPCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    BoDSPCompressorAudioProcessor();
    ~BoDSPCompressorAudioProcessor() override;

    float getOutputMeter() const noexcept { return outputMeter.load(); }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // ---- Compressor DSP ----
    bodsp::EnvelopeFollower  envelopeFollower;
    bodsp::Meter             meter;
    bodsp::SoftClipper       softClipper;

    // Parameter smoothers for zipper-free adjustments
    bodsp::ParameterSmoother thresholdSmoothed;
    bodsp::ParameterSmoother ratioSmoothed;
    bodsp::ParameterSmoother attackSmoothed;
    bodsp::ParameterSmoother releaseSmoothed;
    bodsp::ParameterSmoother makeupSmoothed;
    bodsp::ParameterSmoother mixSmoothed;

    std::atomic<float> outputMeter { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPCompressorAudioProcessor)
};
