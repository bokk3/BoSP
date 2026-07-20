#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <numbers>

//==============================================================================
BoDSPChorusAudioProcessor::BoDSPChorusAudioProcessor()
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

BoDSPChorusAudioProcessor::~BoDSPChorusAudioProcessor() {}

//==============================================================================
void BoDSPChorusAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    const float maxDelaySeconds = kMaxDelayMs * 0.001f;

    // Prepare one DelayLine per voice (mono, channel 0)
    for (int v = 0; v < kVoices; ++v)
    {
        delayLines[v].prepare (sampleRate, maxDelaySeconds, 1);
        delayLines[v].reset();
        feedback[v] = 0.0f;
    }

    // ---- LFO setup ----
    // Voices 0 & 2 are the primary voices (L and R channels).
    // Voices 1 & 3 are offset by 90 degrees for richness.
    for (int v = 0; v < kVoices; ++v)
    {
        lfos[v].prepare (sampleRate);
        lfos[v].setWaveform (bodsp::LFO::LFOWaveform::Sine);
        lfos[v].setBipolar (true);
        lfos[v].reset();
    }
    // 90° phase offset for voices 1 & 3
    lfos[1].setPhaseOffset (0.25f);
    lfos[3].setPhaseOffset (0.25f);
    // L channel pair: voices 0,1 (no channel spread)
    // R channel pair: voices 2,3 (phase-inverted for stereo width)
    lfos[2].setPhaseOffset (0.5f);
    lfos[3].setPhaseOffset (0.75f);

    // ---- Shared DSP ----
    dryWet.prepare (sampleRate);
    dryWet.setMode (bodsp::DryWet::CrossfadeMode::EqualPower);

    rateSmoothed.prepare (sampleRate);
    rateSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    rateSmoothed.setTimeConstant (100.0f); // 100ms rate smoothing (LFO re-tune)

    depthSmoothed.prepare (sampleRate);
    depthSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    depthSmoothed.setTimeConstant (20.0f);

    outputGainSmoothed.prepare (sampleRate);
    outputGainSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    outputGainSmoothed.setTimeConstant (20.0f);

    softClipper.prepare (sampleRate);
    softClipper.setMode (bodsp::SoftClipper::ClipMode::Tanh);

    meter.prepare (sampleRate);

    // Initialise smoothers from current parameter state
    if (auto* p = apvts.getRawParameterValue ("rate"))
    {
        const float r = p->load();
        rateSmoothed.reset (r);
        for (int v = 0; v < kVoices; ++v)
            lfos[v].setRateHz (r);
    }
    if (auto* p = apvts.getRawParameterValue ("depth"))
        depthSmoothed.reset (p->load());
    if (auto* p = apvts.getRawParameterValue ("mix"))
        dryWet.setMix (p->load());
    if (auto* p = apvts.getRawParameterValue ("outputGain"))
        outputGainSmoothed.reset (juce::Decibels::decibelsToGain (p->load()));
}

void BoDSPChorusAudioProcessor::releaseResources() {}

bool BoDSPChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
void BoDSPChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    // Read parameters
    if (auto* p = apvts.getRawParameterValue ("rate"))
        rateSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("depth"))
        depthSmoothed.setTarget (p->load());
    if (auto* p = apvts.getRawParameterValue ("mix"))
        dryWet.setMix (p->load());
    if (auto* p = apvts.getRawParameterValue ("outputGain"))
        outputGainSmoothed.setTarget (juce::Decibels::decibelsToGain (p->load()));

    float fbAmt = 0.2f;
    if (auto* p = apvts.getRawParameterValue ("feedback"))
        fbAmt = p->load();

    float spread = 0.5f;
    if (auto* p = apvts.getRawParameterValue ("spread"))
        spread = p->load();

    bool clip = false;
    if (auto* p = apvts.getRawParameterValue ("softClip"))
        clip = p->load() > 0.5f;

    const double sr = getSampleRate();

    // Base delay in samples: 15ms centre point
    const float baseDelaySamples = 0.015f * static_cast<float> (sr);

    const bool isStereo = (totalNumInputChannels >= 2);
    float* ptrL = buffer.getWritePointer (0);
    float* ptrR = isStereo ? buffer.getWritePointer (1) : nullptr;

    for (int n = 0; n < numSamples; ++n)
    {
        // Advance smoothers
        const float rate  = rateSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();
        const float outG  = outputGainSmoothed.getNextValue();

        // Update LFO rates (smooth re-tune)
        for (int v = 0; v < kVoices; ++v)
            lfos[v].setRateHz (rate);

        // Depth in samples — max is depthMs * sr / 1000
        const float depthSamples = depth * 0.001f * static_cast<float> (sr);

        // Spread offset in samples — pushes voice 2/3 apart from 0/1
        const float spreadSamples = spread * 0.010f * static_cast<float> (sr);

        const float dryL = ptrL[n];
        const float dryR = isStereo ? ptrR[n] : dryL;

        // ---- Left channel: voices 0 & 1 ----
        {
            const float mod0 = lfos[0].getNextValue();  // -1..+1
            const float mod1 = lfos[1].getNextValue();
            const float delaySamples0 = std::max (1.0f, baseDelaySamples + mod0 * depthSamples);
            const float delaySamples1 = std::max (1.0f, baseDelaySamples + mod1 * depthSamples + spreadSamples);

            // Read delayed output
            const float wet0 = delayLines[0].read (0, delaySamples0);
            const float wet1 = delayLines[1].read (0, delaySamples1);

            // Write input + feedback
            delayLines[0].pushSample (0, dryL + feedback[0] * fbAmt);
            delayLines[1].pushSample (0, dryL + feedback[1] * fbAmt);

            feedback[0] = wet0;
            feedback[1] = wet1;

            const float wetL = (wet0 + wet1) * 0.5f;
            float outL = dryWet.process (dryL, wetL) * outG;
            if (clip) outL = softClipper.processSample (outL);
            ptrL[n] = outL;
            meter.processSample (outL);
        }

        // ---- Right channel: voices 2 & 3 (or fold to mono if needed) ----
        if (ptrR != nullptr)
        {
            const float mod2 = lfos[2].getNextValue();
            const float mod3 = lfos[3].getNextValue();
            const float delaySamples2 = std::max (1.0f, baseDelaySamples + mod2 * depthSamples);
            const float delaySamples3 = std::max (1.0f, baseDelaySamples + mod3 * depthSamples + spreadSamples);

            const float wet2 = delayLines[2].read (0, delaySamples2);
            const float wet3 = delayLines[3].read (0, delaySamples3);

            delayLines[2].pushSample (0, dryR + feedback[2] * fbAmt);
            delayLines[3].pushSample (0, dryR + feedback[3] * fbAmt);

            feedback[2] = wet2;
            feedback[3] = wet3;

            const float wetR = (wet2 + wet3) * 0.5f;
            float outR = dryWet.process (dryR, wetR) * outG;
            if (clip) outR = softClipper.processSample (outR);
            ptrR[n] = outR;
            meter.processSample (outR);
        }
    }

    outputMeter.store (meter.getPeakHold());
}

//==============================================================================
void BoDSPChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void BoDSPChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BoDSPChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // LFO rate in Hz
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "rate", "Rate (Hz)",
        juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f),
        1.0f));

    // Modulation depth in ms
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "depth", "Depth (ms)",
        juce::NormalisableRange<float> (0.5f, 15.0f, 0.1f, 0.5f),
        3.0f));

    // Dry/wet mix
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    // Feedback (0 = no feedback, 0.8 = maximum)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "feedback", "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.8f, 0.01f),
        0.2f));

    // Stereo spread: additional delay offset between L and R pairs
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "spread", "Stereo Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    // Output Gain
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f));

    // Soft clipper toggle
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "softClip", "Clipper", false));

    return { params.begin(), params.end() };
}

//==============================================================================
bool BoDSPChorusAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* BoDSPChorusAudioProcessor::createEditor()
{
    return new BoDSPChorusAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoDSPChorusAudioProcessor();
}
