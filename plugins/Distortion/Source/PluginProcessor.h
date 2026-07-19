#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

// shared DSP
#include "../../../shared/DSP/Gain.h"
#include "../../../shared/DSP/WaveShaper.h"
#include "../../../shared/DSP/Tone.h"
#include <atomic>

//==============================================================================
class BoDSPDistortionAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    BoDSPDistortionAudioProcessor();
    ~BoDSPDistortionAudioProcessor() override;

    // Returns the last measured output peak (linear)
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

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters 

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // DSP components
    bodsp::Gain inputGain;
    bodsp::Gain outputGain;
    bodsp::WaveShaper waveShaper;
    bodsp::Tone toneFilter;
    // Smoothed output gain (linear)
    juce::LinearSmoothedValue<float> outputSmoothed { 1.0f };
    static constexpr double outputSmoothingTimeSeconds = 0.02; // 20 ms

    // Dry/Wet
    static constexpr float defaultMix = 1.0f; // 1.0 == fully wet
    float mix { defaultMix };

    // Parameter change caching
    int lastModeIndex { -1 };
    bool lastSoftClip { false };
    // Output meter (peak, linear)
    std::atomic<float> outputMeter { 0.0f };

    // Smoothed parameter for drive to avoid clicks when automating.
    juce::LinearSmoothedValue<float> driveSmoothed { 1.0f };
    static constexpr double driveSmoothingTimeSeconds = 0.02; // 20 ms
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPDistortionAudioProcessor)
};
