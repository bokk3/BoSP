#include "PluginEditor.h"
#include "PluginProcessor.h"

BoDSPDelayAudioProcessorEditor::BoDSPDelayAudioProcessorEditor (BoDSPDelayAudioProcessor& p)
	: juce::AudioProcessorEditor (&p), processorRef (p)
{
	setResizable (true, true);
	setResizeLimits (300, 200, 800, 600);
	setSize (400, 240);

	addAndMakeVisible (timeSlider);
	timeSlider.setTextValueSuffix (" ms");
	timeAttachment.reset (new TimeAttachment (processorRef.apvts, "timeMs", timeSlider));

	addAndMakeVisible (feedbackSlider);
	feedbackAttachment.reset (new FeedbackAttachment (processorRef.apvts, "feedback", feedbackSlider));

	addAndMakeVisible (mixSlider);
	mixAttachment.reset (new MixAttachment (processorRef.apvts, "mix", mixSlider));

	addAndMakeVisible (duckToggle);
	duckAttachment.reset (new DuckAttachment (processorRef.apvts, "duckEnable", duckToggle));

	addAndMakeVisible (meterLabel);
	meterLabel.setText ("Output Meter", juce::dontSendNotification);

	startTimerHz (25);
}

BoDSPDelayAudioProcessorEditor::~BoDSPDelayAudioProcessorEditor() {}

void BoDSPDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll (juce::Colours::darkslategrey);
}

void BoDSPDelayAudioProcessorEditor::resized()
{
	const int margin = 8;
	auto r = getLocalBounds().reduced (margin);
	timeSlider.setBounds (r.removeFromTop (50));
	feedbackSlider.setBounds (r.removeFromTop (50));
	mixSlider.setBounds (r.removeFromTop (50));
	duckToggle.setBounds (r.removeFromTop (30));
	meterLabel.setBounds (r.removeFromTop (30));
}

void BoDSPDelayAudioProcessorEditor::timerCallback()
{
	const float peak = processorRef.getOutputMeter();
	meterLabel.setText (juce::String (peak, 3), juce::dontSendNotification);
}
