#include "PluginProcessor.h"
#include "PluginEditor.h"
// STL
#include <vector>
#include <memory>
// shared DSP
#include "../../../shared/DSP/Gain.h"
#include "../../../shared/DSP/WaveShaper.h"

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
    return 1;
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
    dryWet.prepare (sampleRate);
    meter.prepare (sampleRate);
    softClipper.prepare (sampleRate);
    softClipper.setMode (bodsp::SoftClipper::ClipMode::Tanh);

    // Set up parameter smoothers — 20 ms exponential
    driveSmoothed.prepare (sampleRate);
    driveSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    driveSmoothed.setTimeConstant (20.0f);

    outputSmoothed.prepare (sampleRate);
    outputSmoothed.setMode (bodsp::ParameterSmoother::SmootherMode::Exponential);
    outputSmoothed.setTimeConstant (20.0f);

    // Initialise cached values from APVTS
    if (auto* p = apvts.getRawParameterValue ("drive"))
        driveSmoothed.reset (juce::Decibels::decibelsToGain (p->load()));
    if (auto* p = apvts.getRawParameterValue ("output"))
        outputSmoothed.reset (juce::Decibels::decibelsToGain (p->load()));
    if (auto* p = apvts.getRawParameterValue ("mix"))
        dryWet.setMix (p->load() * 0.01f);

    if (auto* m = apvts.getRawParameterValue ("mode"))
        lastModeIndex = static_cast<int> (m->load());
    if (auto* sc = apvts.getRawParameterValue ("softClip0db"))
        lastSoftClip = sc->load() > 0.5f;
    waveShaper.setMode (static_cast<bodsp::WaveShaper::Mode> (lastModeIndex));
    waveShaper.setSoftClipAt0dB (lastSoftClip);

    if (auto* tonep = apvts.getRawParameterValue ("tone"))
        toneFilter.setCutoffHz (tonep->load());
}

void BoDSPDistortionAudioProcessor::releaseResources()
{
}

bool BoDSPDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void BoDSPDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update drive target (dB → linear)
    if (auto* p = apvts.getRawParameterValue ("drive"))
        driveSmoothed.setTarget (juce::Decibels::decibelsToGain (p->load()));

    // Update output target
    if (auto* p = apvts.getRawParameterValue ("output"))
        outputSmoothed.setTarget (juce::Decibels::decibelsToGain (p->load()));

    // Update waveshaper mode only when changed
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

    // Update dry/wet mix (0–100 % → 0–1)
    if (auto* mixp = apvts.getRawParameterValue ("mix"))
        dryWet.setMix (mixp->load() * 0.01f);

    // Update tone
    if (auto* tonep = apvts.getRawParameterValue ("tone"))
        toneFilter.setCutoffHz (tonep->load());

    const int numSamples = buffer.getNumSamples();

    // Apply input gain per-channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        inputGain.process (buffer.getWritePointer (channel), numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float driveG = driveSmoothed.getNextValue();
        const float outG   = outputSmoothed.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float* ptr = buffer.getWritePointer (channel);
            const float dry = ptr[sample];

            // Wet path: drive → waveshaper → tone filter
            float s = dry * driveG;
            s = waveShaper.process (s);
            s = toneFilter.processSample (channel, s);

            // bodsp::DryWet blend
            float finalOut = dryWet.process (dry, s) * outG;

            // bodsp::SoftClipper (tanh) — only when toggle is on
            if (lastSoftClip)
                finalOut = softClipper.processSample (finalOut);

            ptr[sample] = finalOut;

            // Feed bodsp::Meter
            meter.processSample (finalOut);
        }
    }

    // Publish peak to atomic for GUI
    outputMeter.store (meter.getPeak());
}

//==============================================================================
bool BoDSPDistortionAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BoDSPDistortionAudioProcessor::createEditor()
{
    return new BoDSPDistortionAudioProcessorEditor (*this);
}

//==============================================================================
void BoDSPDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void BoDSPDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
BoDSPDistortionAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "drive", "Drive (dB)",
        juce::NormalisableRange<float> (0.0f, 24.0f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "output", "Output (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        100.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "tone", "Tone",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f),
        10000.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "mode", "Mode",
        juce::StringArray { "Soft", "Tube", "Hard", "Fold" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "softClip0db", "Soft Clip at 0 dB", false));

    return { params.begin(), params.end() };
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoDSPDistortionAudioProcessor();
}
