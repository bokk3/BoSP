#include "PluginProcessor.h"
#include "PluginEditor.h"
// STL
#include <vector>
#include <memory>
// shared DSP
#include "../../../shared/DSP/Gain.h"
#include "../../../shared/DSP/WaveShaper.h"
#include <juce_dsp/juce_dsp.h>

//==============================================================================
BoDSPDistortionAudioProcessor::BoDSPDistortionAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

BoDSPDistortionAudioProcessor::~BoDSPDistortionAudioProcessor()
{
}

//==============================================================================
const juce::String BoDSPDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BoDSPDistortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BoDSPDistortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BoDSPDistortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BoDSPDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BoDSPDistortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BoDSPDistortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BoDSPDistortionAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String BoDSPDistortionAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void BoDSPDistortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void BoDSPDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    inputGain.prepare (sampleRate);
    outputGain.prepare (sampleRate);
    toneFilter.prepare (sampleRate, (int) getTotalNumInputChannels());

    // Reset and initialize smoothing for drive parameter
    driveSmoothed.reset (sampleRate, driveSmoothingTimeSeconds);
    outputSmoothed.reset (sampleRate, outputSmoothingTimeSeconds);
    // Initialize current/target value to the current parameter value
    if (auto* p = apvts.getRawParameterValue ("drive"))
        driveSmoothed.setCurrentAndTargetValue (p->load());

    if (auto* outp = apvts.getRawParameterValue ("output"))
        outputSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outp->load()));

    // initialize cached mode and soft clip flags
    if (auto* m = apvts.getRawParameterValue ("mode"))
        lastModeIndex = static_cast<int> (m->load());
    if (auto* sc = apvts.getRawParameterValue ("softClip0db"))
        lastSoftClip = sc->load() > 0.5f;
    waveShaper.setMode (static_cast<bodsp::WaveShaper::Mode> (lastModeIndex));
    waveShaper.setSoftClipAt0dB (lastSoftClip);

    // Initialize mix and tone values
    if (auto* mixp = apvts.getRawParameterValue ("mix"))
        mix = mixp->load() * 0.01f;
    if (auto* tonep = apvts.getRawParameterValue ("tone"))
        toneFilter.setCutoffHz (tonep->load());
}

void BoDSPDistortionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool BoDSPDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void BoDSPDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    // Read the 'drive' parameter (target) and set the smoother. The smoother
    // will be sampled once per audio sample below (shared across channels).
    float driveTarget = 1.0f;
    if (auto* p = apvts.getRawParameterValue ("drive"))
        driveTarget = p->load();

    // driveTarget supplied in decibels. Convert to linear gain before smoothing.
    const float driveLinearTarget = juce::Decibels::decibelsToGain (driveTarget);
    driveSmoothed.setTargetValue (driveLinearTarget);
    // Update wave shaper mode and soft-clip option only when changed
    if (auto* m = apvts.getRawParameterValue ("mode"))
    {
        const int modeIndex = static_cast<int> (m->load());
        if (modeIndex != lastModeIndex)
        {
            lastModeIndex = modeIndex;
            waveShaper.setMode (static_cast<bodsp::WaveShaper::Mode> (modeIndex));
        }
    }

    if (auto* sc = apvts.getRawParameterValue ("softClip0db"))
    {
        const bool scVal = sc->load() > 0.5f;
        if (scVal != lastSoftClip)
        {
            lastSoftClip = scVal;
            waveShaper.setSoftClipAt0dB (scVal);
        }
    }

    // Update mix and tone per-block (mix is fraction 0..1)
    if (auto* mixp = apvts.getRawParameterValue ("mix"))
        mix = mixp->load() * 0.01f;
    if (auto* tonep = apvts.getRawParameterValue ("tone"))
        toneFilter.setCutoffHz (tonep->load());

    // Update output target (in dB -> linear) for smoothing
    if (auto* outp = apvts.getRawParameterValue ("output"))
    {
        const float outLinear = juce::Decibels::decibelsToGain (outp->load());
        outputSmoothed.setTargetValue (outLinear);
    }

    const int numSamples = buffer.getNumSamples();
    float peak = 0.0f;

    // Apply input gain per-channel (constant multiplier)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        inputGain.process (buffer.getWritePointer (channel), numSamples);

    // Apply per-sample drive smoothing (shared across channels), waveshaper,
    // tone filter, dry/wet mix and smoothed output. We capture the dry
    // sample (post-input gain) and process the wet path, then mix.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float driveG = driveSmoothed.getNextValue();
        const float outG = outputSmoothed.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float* ptr = buffer.getWritePointer (channel);
            const float dry = ptr[sample];

            // Wet path
            float s = dry * driveG;                  // apply drive (pre-waveshaper)
            s = waveShaper.process (s);              // apply waveshaper
            s = toneFilter.processSample (channel, s); // apply tone filter

            // Mix dry/wet and apply output smoothing
            const float outSample = s * mix + dry * (1.0f - mix);
            float finalOut = outSample * outG;

            // Apply final soft clip at 0 dB if enabled. This ensures the
            // last element in the chain prevents output exceeding 0 dBFS.
            if (lastSoftClip)
            {
                // Use tanh to softly limit the signal into [-1, 1]. Do NOT
                // rescale by 1/tanh(1) — that can push values above 1 for
                // large inputs. Plain tanh clamps to < 1.0.
                finalOut = std::tanh (finalOut);
            }

            ptr[sample] = finalOut;

            // track peak across channels/samples
            const float absOut = std::fabs (finalOut);
            if (absOut > peak) peak = absOut;
        }
    }

    // Publish peak to meter (atomic, real-time safe)
    outputMeter.store (peak);
}

//==============================================================================
bool BoDSPDistortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BoDSPDistortionAudioProcessor::createEditor()
{
    return new BoDSPDistortionAudioProcessorEditor (*this);
}

//==============================================================================
void BoDSPDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void BoDSPDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BoDSPDistortionAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Drive in dB: 0 -> +24 dB (musician-friendly). We'll convert to linear before use.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "drive", "Drive (dB)",
        juce::NormalisableRange<float> (0.0f, 24.0f),
        0.0f));

    // Output gain in dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "output", "Output (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f),
        0.0f));

    // Mix (dry/wet) percentage
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        100.0f));

    // Tone control: cutoff frequency in Hz
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "tone", "Tone",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f),
        10000.0f));

    // Mode selector: Soft, Tube, Hard, Fold
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Mode",
        juce::StringArray { "Soft", "Tube", "Hard", "Fold" },
        0));

    // Soft clip at 0 dB toggle
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "softClip0db", "Soft Clip at 0 dB", false));

    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoDSPDistortionAudioProcessor();
}
