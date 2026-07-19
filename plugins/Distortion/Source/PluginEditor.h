#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

//==============================================================================
class BoDSPDistortionAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BoDSPDistortionAudioProcessorEditor (BoDSPDistortionAudioProcessor&);
    ~BoDSPDistortionAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BoDSPDistortionAudioProcessor& processorRef;

    // UI controls
    juce::Slider driveSlider;
    using DriveAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<DriveAttachment> driveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPDistortionAudioProcessorEditor)
};
