#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <cmath>

//==============================================================================
class BoDSPReverbAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
	explicit BoDSPReverbAudioProcessorEditor (BoDSPReverbAudioProcessor&);
	~BoDSPReverbAudioProcessorEditor() override;

	void timerCallback() override;
	void paint (juce::Graphics&) override;
	void resized() override;

private:
	BoDSPReverbAudioProcessor& processorRef;

	// --- Sliders ---
	juce::Slider roomSizeSlider, dampingSlider, widthSlider,
	             mixSlider, preDelaySlider, lpSlider, hpSlider;

	using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<Attach> roomSizeAttach, dampingAttach, widthAttach,
	                        mixAttach, preDelayAttach, lpAttach, hpAttach;

	// --- Labels ---
	juce::Label roomSizeLabel, dampingLabel, widthLabel,
	            mixLabel, preDelayLabel, lpLabel, hpLabel;

	// --- Output meter ---
	class MeterComponent : public juce::Component
	{
	public:
		MeterComponent() = default;

		void setLevel (float v) noexcept { level = v; repaint(); }

		void paint (juce::Graphics& g) override
		{
			const auto bounds = getLocalBounds().toFloat();
			g.fillAll (juce::Colour (0xff1a1a2e));

			const float lin = std::max (level, 1e-9f);
			const float db  = 20.0f * std::log10 (lin);
			const float minDb = -60.0f, maxDb = 6.0f;
			float frac = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
			const float fillW = bounds.getWidth() * frac;

			juce::Colour fillCol;
			if (db >  0.0f) fillCol = juce::Colour (0xffaa00ff);
			else if (db > -3.0f) fillCol = juce::Colour (0xffff4444);
			else if (db > -6.0f) fillCol = juce::Colour (0xffffe066);
			else                 fillCol = juce::Colour (0xff44ccaa);

			g.setColour (fillCol);
			g.fillRect (bounds.withWidth (fillW));
			g.setColour (juce::Colours::black);
			g.drawRect (bounds);
			g.setColour (juce::Colours::white);
			g.setFont (11.0f);
			g.drawText (juce::String (db, 1) + " dB",
			            getLocalBounds(), juce::Justification::centredRight, false);
		}
	private:
		float level { 0.0f };
	} meter;

	// Helpers
	void setupKnob (juce::Slider& s, juce::Label& lbl, const juce::String& text);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPReverbAudioProcessorEditor)
};
