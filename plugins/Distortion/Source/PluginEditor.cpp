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
    driveSlider.setBounds (40, 40, 120, 120);
    outputSlider.setBounds (180, 40, 120, 120);
    modeBox.setBounds (320, 80, 120, 24);
}
