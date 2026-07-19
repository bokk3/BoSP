#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <vector>
#include <cmath>

//==============================================================================
BoDSPFilterAudioProcessor::BoDSPFilterAudioProcessor()
	: AudioProcessor (BusesProperties()
					#if ! JucePlugin_IsMidiEffect
					 #if ! JucePlugin_IsSynth
					  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
					 #endif
					  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
					#endif
					  ),
	  apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

BoDSPFilterAudioProcessor::~BoDSPFilterAudioProcessor() {}

bool BoDSPFilterAudioProcessor::acceptsMidi() const { return false; }
bool BoDSPFilterAudioProcessor::producesMidi() const { return false; }
bool BoDSPFilterAudioProcessor::isMidiEffect() const { return false; }

//==============================================================================
void BoDSPFilterAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
	const int numCh = static_cast<int> (getTotalNumInputChannels());
	filter.prepare (sampleRate, numCh);

	if (auto* p = apvts.getRawParameterValue ("mode"))
	{
		lastModeIndex = static_cast<int> (p->load());
		filter.setMode (static_cast<bodsp::BiquadFilter::Mode> (lastModeIndex));
	}
	if (auto* p = apvts.getRawParameterValue ("freq"))   filter.setFrequency (p->load());
	if (auto* p = apvts.getRawParameterValue ("q"))      filter.setQ (p->load());
	if (auto* p = apvts.getRawParameterValue ("gainDb")) filter.setGainDb (p->load());
}

void BoDSPFilterAudioProcessor::releaseResources() {}

bool BoDSPFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused (layouts);
	return true;
#else
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif
	return true;
#endif
}

//==============================================================================
void BoDSPFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midiMessages*/)
{
	juce::ScopedNoDenormals noDenormals;

	const int totalNumInputChannels  = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	const int numSamples = buffer.getNumSamples();

	for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, numSamples);

	// Update parameters per-block
	if (auto* p = apvts.getRawParameterValue ("mode"))
	{
		const int modeIdx = static_cast<int> (p->load());
		if (modeIdx != lastModeIndex)
		{
			lastModeIndex = modeIdx;
			filter.setMode (static_cast<bodsp::BiquadFilter::Mode> (modeIdx));
		}
	}
	if (auto* p = apvts.getRawParameterValue ("freq"))   filter.setFrequency (p->load());
	if (auto* p = apvts.getRawParameterValue ("q"))      filter.setQ (p->load());
	if (auto* p = apvts.getRawParameterValue ("gainDb")) filter.setGainDb (p->load());

	// Get mix parameter
	float mix = 1.0f;
	if (auto* p = apvts.getRawParameterValue ("mix")) mix = p->load();

	float peak = 0.0f;

	for (int ch = 0; ch < totalNumInputChannels; ++ch)
	{
		float* ptr = buffer.getWritePointer (ch);
		for (int n = 0; n < numSamples; ++n)
		{
			const float dry = ptr[n];
			const float wet = filter.processSample (ch, dry);
			const float out = dry * (1.0f - mix) + wet * mix;
			ptr[n] = out;
			peak = std::max (peak, std::fabs (out));
		}
	}

	outputMeter.store (peak);
}

//==============================================================================
void BoDSPFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	juce::ignoreUnused (destData);
}

void BoDSPFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BoDSPFilterAudioProcessor::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	// Filter mode
	params.push_back (std::make_unique<juce::AudioParameterChoice> (
		"mode", "Mode",
		juce::StringArray { "Lowpass", "Highpass", "Low Shelf", "High Shelf", "Notch" },
		0));

	// Cutoff / centre frequency
	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"freq", "Frequency",
		juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f),
		1000.0f));

	// Resonance / Q
	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"q", "Q / Resonance",
		juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f),
		0.707f));

	// Shelf / peak gain (only meaningful for LowShelf / HighShelf)
	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"gainDb", "Gain (dB)",
		juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
		0.0f));

	// Dry/wet mix
	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"mix", "Mix",
		juce::NormalisableRange<float> (0.0f, 1.0f),
		1.0f));

	return { params.begin(), params.end() };
}

//==============================================================================
bool BoDSPFilterAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BoDSPFilterAudioProcessor::createEditor()
{
	return new BoDSPFilterAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new BoDSPFilterAudioProcessor();
}
