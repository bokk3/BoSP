#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

//==============================================================================
BoDSPCompressorAudioProcessor::BoDSPCompressorAudioProcessor()
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

BoDSPCompressorAudioProcessor::~BoDSPCompressorAudioProcessor() {}

//==============================================================================
void BoDSPCompressorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    envelopeFollower.prepare (sampleRate);
    envelopeFollower.setMode (bodsp::EnvelopeFollower::FollowerMode::Peak); // Fast peak response

    meter.prepare (sampleRate);
    softClipper.prepare (sampleRate);
    softClipper.setMode (bodsp::SoftClipper::ClipMode::Tanh);

    // Initialize smoothers
    thresholdSmoothed.prepare (sampleRate);
    thresholdSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    thresholdSmoothed.setTimeConstant (10.0f);

    ratioSmoothed.prepare (sampleRate);
    ratioSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    ratioSmoothed.setTimeConstant (10.0f);

    attackSmoothed.prepare (sampleRate);
    attackSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    attackSmoothed.setTimeConstant (10.0f);

    releaseSmoothed.prepare (sampleRate);
    releaseSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    releaseSmoothed.setTimeConstant (10.0f);

    makeupSmoothed.prepare (sampleRate);
    makeupSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    makeupSmoothed.setTimeConstant (10.0f);

    mixSmoothed.prepare (sampleRate);
    mixSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    mixSmoothed.setTimeConstant (10.0f);

    // Reset smoothers to target
    if (auto* p = apvts.getRawParameterValue ("threshold"))   thresholdSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("ratio"))       ratioSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("attack"))      attackSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("release"))     releaseSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("makeup"))      makeupSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("mix"))         mixSmoothed.reset (p->load());
}

void BoDSPCompressorAudioProcessor::releaseResources() {}

bool BoDSPCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void BoDSPCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // Update targets
    if (auto* p = apvts.getRawParameterValue ("threshold"))   thresholdSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("ratio"))       ratioSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("attack"))      attackSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("release"))     releaseSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("makeup"))      makeupSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("mix"))         mixSmoothed.setTarget (p->load());

    bool clip = false;
    if (auto* p = apvts.getRawParameterValue ("softClip"))
        clip = p->load() > 0.5f;

    const bool isStereo = (totalNumInputChannels >= 2);
    float* ptrL = buffer.getWritePointer (0);
    float* ptrR = isStereo ? buffer.getWritePointer (1) : nullptr;

    // Pre-seed the envelope ballistics with current parameter values at block start.
    // We still advance the smoothers per-sample, but we only actually update the
    // ballistic coefficients once per block to avoid per-sample coefficient recomputation.
    if (auto* p = apvts.getRawParameterValue ("attack"))
        envelopeFollower.setAttackMs (p->load());
    if (auto* p = apvts.getRawParameterValue ("release"))
        envelopeFollower.setReleaseMs (p->load());

    for (int n = 0; n < numSamples; ++n)
    {
        // Advance smoothers
        const float threshold = thresholdSmoothed.getNextValue();
        const float ratio     = ratioSmoothed.getNextValue();
        const float attack    = attackSmoothed.getNextValue();
        const float release   = releaseSmoothed.getNextValue();
        const float makeup    = makeupSmoothed.getNextValue();
        const float mix       = mixSmoothed.getNextValue();

        // Suppress unused variable warnings for smoothed ballistics
        // (they converge toward target but we apply at block-rate for performance)
        juce::ignoreUnused (attack, release);

        const float dryL = ptrL[n];
        const float dryR = isStereo ? ptrR[n] : dryL;

        // Stereo-linked peak detection
        const float sidechain = std::max (std::abs (dryL), std::abs (dryR));
        const float env = envelopeFollower.processSample (sidechain);

        // Convert envelope to dB
        const float envDb = (env > 1e-9f) ? 20.0f * std::log10 (env) : -120.0f;

        // Gain reduction in dB
        float grDb = 0.0f;
        if (envDb > threshold)
        {
            grDb = (threshold - envDb) * (1.0f - 1.0f / ratio);
        }

        // Convert back to linear gain reduction multiplier
        const float grLinear = std::pow (10.0f, grDb * 0.05f);

        // Apply compression (wet path)
        float wetL = dryL * grLinear;
        float wetR = dryR * grLinear;

        // Apply makeup gain (makeup is in dB, convert to linear)
        const float makeupLinear = std::pow (10.0f, makeup * 0.05f);
        wetL *= makeupLinear;
        wetR *= makeupLinear;

        // Dry/wet blend
        float outL = dryL * (1.0f - mix) + wetL * mix;
        float outR = dryR * (1.0f - mix) + wetR * mix;

        // Soft clipper safety
        if (clip)
        {
            outL = softClipper.processSample (outL);
            outR = softClipper.processSample (outR);
        }

        ptrL[n] = outL;
        meter.processSample (outL);
        if (ptrR != nullptr)
        {
            ptrR[n] = outR;
            meter.processSample (outR);
        }
    }

    outputMeter.store (meter.getPeakHold());
}

//==============================================================================
void BoDSPCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void BoDSPCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BoDSPCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Threshold in dB: -60 dB to 0 dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "threshold", "Threshold (dB)",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -12.0f));

    // Ratio: 1:1 to 20:1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ratio", "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f),
        4.0f));

    // Attack in ms: 0.1 ms to 200 ms
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack (ms)",
        juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.3f),
        10.0f));

    // Release in ms: 10 ms to 2000 ms
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release (ms)",
        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.3f),
        100.0f));

    // Makeup gain in dB: -24 dB to +24 dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "makeup", "Makeup (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f));

    // Dry/wet mix: 0.0 to 1.0
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        1.0f));

    // Soft clip toggle
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "softClip", "Clipper", false));

    return { params.begin(), params.end() };
}

//==============================================================================
bool BoDSPCompressorAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BoDSPCompressorAudioProcessor::createEditor()
{
    return new BoDSPCompressorAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoDSPCompressorAudioProcessor();
}
