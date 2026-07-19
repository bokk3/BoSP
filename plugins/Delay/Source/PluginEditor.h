#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class BoDSPDelayAudioProcessorEditor  : public juce::AudioProcessorEditor,
									   private juce::Timer
{
public:
	explicit BoDSPDelayAudioProcessorEditor (BoDSPDelayAudioProcessor&);
	~BoDSPDelayAudioProcessorEditor() override;

	void paint (juce::Graphics&) override;
	void resized() override;
	void timerCallback() override;

private:
	BoDSPDelayAudioProcessor& processorRef;

	juce::Slider timeSlider;
	using TimeAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<TimeAttachment> timeAttachment;

	juce::Slider feedbackSlider;
	using FeedbackAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<FeedbackAttachment> feedbackAttachment;

	juce::Slider mixSlider;
	using MixAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<MixAttachment> mixAttachment;

	juce::ToggleButton duckToggle { "Auto Duck" };
	using DuckAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
	std::unique_ptr<DuckAttachment> duckAttachment;

	juce::Label meterLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPDelayAudioProcessorEditor)
};
