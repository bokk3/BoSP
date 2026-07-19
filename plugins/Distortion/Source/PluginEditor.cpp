#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BoDSPDistortionAudioProcessorEditor::BoDSPDistortionAudioProcessorEditor (BoDSPDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // Drive slider
    driveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    driveSlider.setTextValueSuffix (" dB");
    driveSlider.setRange (0.0, 24.0, 0.01);
    addAndMakeVisible (driveSlider);

    driveAttachment = std::make_unique<DriveAttachment> (processorRef.apvts, "drive", driveSlider);

    // Output slider
    outputSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    outputSlider.setTextValueSuffix (" dB");
    outputSlider.setRange (-24.0, 24.0, 0.01);
    addAndMakeVisible (outputSlider);
    outputAttachment = std::make_unique<OutputAttachment> (processorRef.apvts, "output", outputSlider);

    // Mode combo box
    modeBox.addItem ("Soft", 1);
    modeBox.addItem ("Tube", 2);
    modeBox.addItem ("Hard", 3);
    modeBox.addItem ("Fold", 4);
    addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<ModeAttachment> (processorRef.apvts, "mode", modeBox);

    // Soft clip toggle
    addAndMakeVisible (softClipToggle);
    softClipAttachment = std::make_unique<SoftClipAttachment> (processorRef.apvts, "softClip0db", softClipToggle);

    // Mix slider (dry/wet)
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    mixSlider.setTextValueSuffix (" %");
    mixSlider.setRange (0.0, 100.0, 0.1);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<MixAttachment> (processorRef.apvts, "mix", mixSlider);

    // Tone slider
    toneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    toneSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    toneSlider.setTextValueSuffix (" Hz");
    toneSlider.setRange (20.0, 20000.0, 1.0);
    toneSlider.setSkewFactorFromMidPoint (1000.0); // logarithmic feel
    addAndMakeVisible (toneSlider);
    toneAttachment = std::make_unique<ToneAttachment> (processorRef.apvts, "tone", toneSlider);

    // Meter
    addAndMakeVisible (meter);

    // Start timer to refresh meter (30 Hz)
    startTimerHz (30);
}

BoDSPDistortionAudioProcessorEditor::~BoDSPDistortionAudioProcessorEditor()
{
}

//==============================================================================
void BoDSPDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw plugin name instead of placeholder text
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawFittedText ("BoDSP Distortion", getLocalBounds().withY (10).withHeight (30), juce::Justification::centredTop, 1);
}

void BoDSPDistortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    driveSlider.setBounds (20, 40, 110, 110);
    outputSlider.setBounds (140, 40, 110, 110);
    mixSlider.setBounds (260, 40, 110, 110);
    toneSlider.setBounds (380, 40, 110, 110);
    modeBox.setBounds (20, 160, 140, 24);
    softClipToggle.setBounds (180, 160, 160, 24);
    meter.setBounds (20, 200, getWidth() - 40, 24);
}

void BoDSPDistortionAudioProcessorEditor::timerCallback()
{
    // Poll processor meter (atomic)
    const float v = processorRef.getOutputMeter();
    meter.setLevel (v);
}
