#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <vector>

//==============================================================================

BoDSPReverbAudioProcessor::BoDSPReverbAudioProcessor()
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

BoDSPReverbAudioProcessor::~BoDSPReverbAudioProcessor() {}

bool BoDSPReverbAudioProcessor::acceptsMidi() const { return false; }
bool BoDSPReverbAudioProcessor::producesMidi() const { return false; }
bool BoDSPReverbAudioProcessor::isMidiEffect() const { return false; }

void BoDSPReverbAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
	const int numCh = static_cast<int> (getTotalNumInputChannels());
	reverb.prepare (sampleRate, numCh);

	// Apply initial parameter values
	if (auto* p = apvts.getRawParameterValue ("roomSize"))  reverb.setRoomSize (p->load());
	if (auto* p = apvts.getRawParameterValue ("damping"))   reverb.setDamping (p->load());
	if (auto* p = apvts.getRawParameterValue ("width"))     reverb.setWidth (p->load());
	if (auto* p = apvts.getRawParameterValue ("mix"))       reverb.setMix (p->load());
	if (auto* p = apvts.getRawParameterValue ("preDelay"))  reverb.setPreDelayMs (p->load());
	if (auto* p = apvts.getRawParameterValue ("lpCutoff"))  reverb.setLPCutoff (p->load());
	if (auto* p = apvts.getRawParameterValue ("hpCutoff"))  reverb.setHPCutoff (p->load());
}

void BoDSPReverbAudioProcessor::releaseResources() {}

bool BoDSPReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void BoDSPReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
	juce::ScopedNoDenormals noDenormals;

	const int totalNumInputChannels  = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	const int numSamples = buffer.getNumSamples();

	for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, numSamples);

	// Update parameters per-block
	if (auto* p = apvts.getRawParameterValue ("roomSize"))  reverb.setRoomSize (p->load());
	if (auto* p = apvts.getRawParameterValue ("damping"))   reverb.setDamping (p->load());
	if (auto* p = apvts.getRawParameterValue ("width"))     reverb.setWidth (p->load());
	if (auto* p = apvts.getRawParameterValue ("mix"))       reverb.setMix (p->load());
	if (auto* p = apvts.getRawParameterValue ("preDelay"))  reverb.setPreDelayMs (p->load());
	if (auto* p = apvts.getRawParameterValue ("lpCutoff"))  reverb.setLPCutoff (p->load());
	if (auto* p = apvts.getRawParameterValue ("hpCutoff"))  reverb.setHPCutoff (p->load());

	// Get pointers for left and right channels
	float* ptrL = buffer.getWritePointer (0);
	float* ptrR = (totalNumInputChannels > 1) ? buffer.getWritePointer (1) : nullptr;

	float peak = 0.0f;

	for (int n = 0; n < numSamples; ++n)
	{
		const float inL = ptrL[n];
		const float inR = (ptrR != nullptr) ? ptrR[n] : inL;

		float outL = 0.0f;
		float outR = 0.0f;
		reverb.processSample (inL, inR, outL, outR);

		ptrL[n] = outL;
		if (ptrR != nullptr)
			ptrR[n] = outR;

		peak = std::max (peak, std::max (std::fabs (outL), std::fabs (outR)));
	}

	outputMeter.store (peak);
}

void BoDSPReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	juce::ignoreUnused (destData);
}

void BoDSPReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout BoDSPReverbAudioProcessor::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"roomSize", "Room Size",
		juce::NormalisableRange<float> (0.0f, 0.98f), 0.5f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"damping", "Damping",
		juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"width", "Width",
		juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"mix", "Mix",
		juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"preDelay", "Pre-Delay (ms)",
		juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f), 10.0f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"lpCutoff", "LP Cutoff",
		juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.5f), 15000.0f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> (
		"hpCutoff", "HP Cutoff",
		juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 20.0f));

	return { params.begin(), params.end() };
}

//==============================================================================
bool BoDSPReverbAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BoDSPReverbAudioProcessor::createEditor()
{
	return new BoDSPReverbAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new BoDSPReverbAudioProcessor();
}
