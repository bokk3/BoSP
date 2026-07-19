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
    addAndMakeVisible (driveSlider);

    driveAttachment = std::make_unique<DriveAttachment> (processorRef.apvts, "drive", driveSlider);
}

BoDSPDistortionAudioProcessorEditor::~BoDSPDistortionAudioProcessorEditor()
{
}

//==============================================================================
void BoDSPDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void BoDSPDistortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    driveSlider.setBounds (40, 40, 120, 120);
}
