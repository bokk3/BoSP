#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <cmath>

//==============================================================================
class BoDSPFilterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
	explicit BoDSPFilterAudioProcessorEditor (BoDSPFilterAudioProcessor&);
	~BoDSPFilterAudioProcessorEditor() override;

	void timerCallback() override;
	void paint (juce::Graphics&) override;
	void resized() override;

private:
	BoDSPFilterAudioProcessor& processorRef;

	// Mode selector
	juce::ComboBox modeBox;
	using ModeAttach = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	std::unique_ptr<ModeAttach> modeAttach;

	// Knobs
	juce::Slider freqSlider, qSlider, gainSlider, mixSlider;
	using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<Attach> freqAttach, qAttach, gainAttach, mixAttach;

	// Labels
	juce::Label freqLabel, qLabel, gainLabel, mixLabel;

	// Output meter
	class MeterComponent : public juce::Component
	{
	public:
		MeterComponent() = default;
		void setLevel (float v) noexcept { level = v; repaint(); }

		void paint (juce::Graphics& g) override
		{
			const auto b = getLocalBounds().toFloat();
			g.fillAll (juce::Colour (0xff0d1a1a));

			const float lin = std::max (level, 1e-9f);
			const float db  = 20.0f * std::log10 (lin);
			float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);

			juce::Colour col;
			if (db > 0.0f)        col = juce::Colour (0xffaa00ff);
			else if (db > -3.0f)  col = juce::Colour (0xffff4444);
			else if (db > -6.0f)  col = juce::Colour (0xffffe066);
			else                   col = juce::Colour (0xff33ccaa);

			g.setColour (col);
			g.fillRect (b.withWidth (b.getWidth() * frac));
			g.setColour (juce::Colours::black);
			g.drawRect (b);
			g.setColour (juce::Colours::white);
			g.setFont (11.0f);
			g.drawText (juce::String (db, 1) + " dB", getLocalBounds(),
			            juce::Justification::centredRight, false);
		}
	private:
		float level { 0.0f };
	} meter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPFilterAudioProcessorEditor)
};
