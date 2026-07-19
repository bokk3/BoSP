#include "PluginProcessor.h"
#include "PluginEditor.h"

// STL
#include <vector>

//==============================================================================

BoDSPDelayAudioProcessor::BoDSPDelayAudioProcessor()
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

BoDSPDelayAudioProcessor::~BoDSPDelayAudioProcessor() {}

bool BoDSPDelayAudioProcessor::acceptsMidi() const { return false; }
bool BoDSPDelayAudioProcessor::producesMidi() const { return false; }
bool BoDSPDelayAudioProcessor::isMidiEffect() const { return false; }

void BoDSPDelayAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
	const int numCh = static_cast<int> (getTotalNumInputChannels());
	inputGain.prepare (sampleRate);
	outputGain.prepare (sampleRate);
	delay.prepare (sampleRate, numCh, 5.0f);

	if (auto* p = apvts.getRawParameterValue ("timeMs"))
		delay.setTimeMs (p->load());
	if (auto* f = apvts.getRawParameterValue ("feedback"))
		delay.setFeedback (f->load());
	if (auto* m = apvts.getRawParameterValue ("mix"))
		delay.setMix (m->load());
	if (auto* lp = apvts.getRawParameterValue ("lp"))
		delay.setLP (lp->load());
	if (auto* hp = apvts.getRawParameterValue ("hp"))
		delay.setHP (hp->load());
	if (auto* de = apvts.getRawParameterValue ("duckEnable"))
		delay.setDuckEnable (de->load() > 0.5f);
	if (auto* th = apvts.getRawParameterValue ("duckThreshold"))
	{
		float thr = th->load();
		float ratio = 4.0f;
		if (auto* r = apvts.getRawParameterValue ("duckRatio")) ratio = r->load();
		float depth = -18.0f;
		if (auto* d = apvts.getRawParameterValue ("duckDepth")) depth = d->load();
		float attack = 10.0f;
		if (auto* a = apvts.getRawParameterValue ("duckAttack")) attack = a->load();
		float release = 200.0f;
		if (auto* rel = apvts.getRawParameterValue ("duckRelease")) release = rel->load();
		delay.setDuckParams (thr, ratio, depth, attack, release);
	}
}

void BoDSPDelayAudioProcessor::releaseResources() {}

bool BoDSPDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void BoDSPDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
	juce::ignoreUnused (/*midiMessages*/);
	juce::ScopedNoDenormals noDenormals;

	const int totalNumInputChannels  = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	const int numSamples = buffer.getNumSamples();

	for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, numSamples);

	// Apply input gain
	for (int ch = 0; ch < totalNumInputChannels; ++ch)
		inputGain.process (buffer.getWritePointer (ch), numSamples);

	float peak = 0.0f;

	// Update parameters that may change per-block
	if (auto* m = apvts.getRawParameterValue ("mix"))
		delay.setMix (m->load());

	// sample-by-sample processing to support ducking based on input level
	for (int n = 0; n < numSamples; ++n)
	{
		// measure input across channels (max abs)
		float inLevel = 0.0f;
		for (int ch = 0; ch < totalNumInputChannels; ++ch)
			inLevel = std::max (inLevel, std::fabs (buffer.getReadPointer (ch)[n]));

		for (int ch = 0; ch < totalNumInputChannels; ++ch)
		{
			float* ptr = buffer.getWritePointer (ch);
			const float dry = ptr[n];
			const float processed = delay.processSample (ch, dry, inLevel);
			ptr[n] = processed;
			peak = std::max (peak, std::fabs (processed));
		}
	}

	outputMeter.store (peak);
}

void BoDSPDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	juce::ignoreUnused (destData);
}

void BoDSPDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout BoDSPDelayAudioProcessor::createParameterLayout()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	params.push_back (std::make_unique<juce::AudioParameterFloat> ("timeMs", "Time (ms)",
		juce::NormalisableRange<float> (1.0f, 5000.0f, 0.1f, 0.5f), 500.0f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback",
		juce::NormalisableRange<float> (0.0f, 0.99f), 0.45f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix",
		juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> ("lp", "LP Cutoff",
		juce::NormalisableRange<float> (50.0f, 20000.0f, 1.0f, 0.5f), 8000.0f));

	params.push_back (std::make_unique<juce::AudioParameterFloat> ("hp", "HP Cutoff",
		juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 20.0f));

	params.push_back (std::make_unique<juce::AudioParameterBool> ("duckEnable", "Auto Duck", true));
	params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckThreshold", "Duck Threshold (dB)",
		juce::NormalisableRange<float> (-60.0f, 0.0f), -18.0f));
	params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckRatio", "Duck Ratio",
		juce::NormalisableRange<float> (1.0f, 20.0f), 4.0f));
	params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckDepth", "Duck Depth (dB)",
		juce::NormalisableRange<float> (-40.0f, 0.0f), -18.0f));
	params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckAttack", "Duck Attack (ms)",
		juce::NormalisableRange<float> (0.1f, 200.0f), 10.0f));
	params.push_back (std::make_unique<juce::AudioParameterFloat> ("duckRelease", "Duck Release (ms)",
		juce::NormalisableRange<float> (10.0f, 2000.0f), 200.0f));

	return { params.begin(), params.end() };
}

//==============================================================================
// Editor factory
bool BoDSPDelayAudioProcessor::hasEditor() const
{
	return true;
}

juce::AudioProcessorEditor* BoDSPDelayAudioProcessor::createEditor()
{
	return new BoDSPDelayAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new BoDSPDelayAudioProcessor();
}
