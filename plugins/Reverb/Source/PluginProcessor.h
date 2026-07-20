#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

// shared DSP
#include "../../../shared/DSP/Reverb.h"
#include "../../../shared/DSP/SoftClipper.h"
#include "../../../shared/DSP/Meter.h"
#include <atomic>

class BoDSPReverbAudioProcessor  : public juce::AudioProcessor
{
public:
	BoDSPReverbAudioProcessor();
	~BoDSPReverbAudioProcessor() override;

	float getOutputMeter() const noexcept { return outputMeter.load(); }

	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
	void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	using AudioProcessor::processBlock;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	const juce::String getName() const override { return JucePlugin_Name; }
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override { return 2.0; }

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
	// DSP
	bodsp::Reverb      reverb;
	bodsp::SoftClipper softClipper;
	bodsp::Meter       meter;

	std::atomic<float> outputMeter { 0.0f };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPReverbAudioProcessor)
};
