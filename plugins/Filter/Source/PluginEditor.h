#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
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

	// Toggles
	juce::ToggleButton clipToggle { "Clipper" };
	using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
	std::unique_ptr<ButtonAttach> clipAttach;

	// Output meter
	class MeterComponent : public juce::Component
	{
	public:
		MeterComponent() = default;
		void setLevel (float v) noexcept { level = v; repaint(); }

		void paint (juce::Graphics& g) override
		{
			const auto bounds = getLocalBounds().toFloat();
			g.fillAll (juce::Colour (0xff0d0d12));

			const float lin = std::max (level, 1e-9f);
			const float db  = 20.0f * std::log10 (lin);
			const float minDb = -60.0f, maxDb = 6.0f;
			float frac = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
			const float fillW = bounds.getWidth() * frac;

			juce::ColourGradient meterGrad (juce::Colour (0xff39ff14), 0.0f, 0.0f,
			                               juce::Colour (0xffff3300), bounds.getWidth(), 0.0f, false);
			meterGrad.addColour (0.7f, juce::Colour (0xffffe066));
			g.setGradientFill (meterGrad);
			g.fillRect (bounds.withWidth (fillW));

			g.setColour (juce::Colour (0xff1f1f26));
			g.drawRect (bounds, 1.5f);

			g.setColour (juce::Colours::white);
			g.setFont (juce::Font (11.0f, juce::Font::bold));
			g.drawText (juce::String (db, 1) + " dB", getLocalBounds().reduced (4, 0),
			            juce::Justification::centredRight, false);
		}
	private:
		float level { 0.0f };
	} meter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPFilterAudioProcessorEditor)
};
