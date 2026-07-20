#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

// shared DSP
#include "../../../shared/DSP/DelayLine.h"
#include "../../../shared/DSP/LFO.h"
#include "../../../shared/DSP/DryWet.h"
#include "../../../shared/DSP/ParameterSmoother.h"
#include "../../../shared/DSP/SoftClipper.h"
#include "../../../shared/DSP/Meter.h"

//==============================================================================
class BoDSPChorusAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    BoDSPChorusAudioProcessor();
    ~BoDSPChorusAudioProcessor() override;

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
    double getTailLengthSeconds() const override { return 0.05; }

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
    // -------------------------------------------------------------------
    // Chorus DSP architecture:
    //   2 voices per channel (4 total), each driven by its own LFO.
    //   Voices per channel share the same base delay with spread offset.
    //   Voice 0/1 → L channel   |   Voice 2/3 → R channel
    //   Each pair has LFOs 90° apart for width.
    // -------------------------------------------------------------------
    static constexpr int kVoices = 4;   // 2 per channel

    bodsp::DelayLine       delayLines[kVoices];
    bodsp::LFO             lfos[kVoices];
    bodsp::DryWet          dryWet;
    bodsp::ParameterSmoother rateSmoothed;
    bodsp::ParameterSmoother depthSmoothed;
    bodsp::SoftClipper     softClipper;
    bodsp::Meter           meter;

    // Maximum modulated delay: base (30ms) + max depth (15ms)
    static constexpr float kMaxDelayMs = 45.0f;

    // Feedback sample memory per voice
    float feedback[kVoices] { 0.0f, 0.0f, 0.0f, 0.0f };

    std::atomic<float> outputMeter { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPChorusAudioProcessor)
};
