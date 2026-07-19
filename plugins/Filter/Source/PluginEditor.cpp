#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupKnob (juce::Slider& s, juce::Label& lbl,
                       const juce::String& text, juce::Component* parent)
{
	s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
	s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
	parent->addAndMakeVisible (s);

	lbl.setText (text, juce::dontSendNotification);
	lbl.setJustificationType (juce::Justification::centred);
	lbl.setFont (juce::Font (12.0f));
	lbl.setColour (juce::Label::textColourId, juce::Colours::white);
	parent->addAndMakeVisible (lbl);
}

//==============================================================================
BoDSPFilterAudioProcessorEditor::BoDSPFilterAudioProcessorEditor (BoDSPFilterAudioProcessor& p)
	: AudioProcessorEditor (&p), processorRef (p)
{
	setSize (420, 260);

	auto& apvts = processorRef.apvts;

	// Mode combo box
	modeBox.addItem ("Lowpass",    1);
	modeBox.addItem ("Highpass",   2);
	modeBox.addItem ("Low Shelf",  3);
	modeBox.addItem ("High Shelf", 4);
	modeBox.addItem ("Notch",      5);
	addAndMakeVisible (modeBox);
	modeAttach = std::make_unique<ModeAttach> (apvts, "mode", modeBox);

	// Knobs
	setupKnob (freqSlider, freqLabel, "Frequency", this);
	freqSlider.setTextValueSuffix (" Hz");
	freqSlider.setSkewFactorFromMidPoint (1000.0);
	freqAttach = std::make_unique<Attach> (apvts, "freq", freqSlider);

	setupKnob (qSlider, qLabel, "Q", this);
	qAttach = std::make_unique<Attach> (apvts, "q", qSlider);

	setupKnob (gainSlider, gainLabel, "Gain", this);
	gainSlider.setTextValueSuffix (" dB");
	gainAttach = std::make_unique<Attach> (apvts, "gainDb", gainSlider);

	setupKnob (mixSlider, mixLabel, "Mix", this);
	mixAttach = std::make_unique<Attach> (apvts, "mix", mixSlider);

	addAndMakeVisible (meter);
	startTimerHz (30);
}

BoDSPFilterAudioProcessorEditor::~BoDSPFilterAudioProcessorEditor() {}

//==============================================================================
void BoDSPFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
	// Dark teal-to-midnight gradient
	juce::ColourGradient grad (juce::Colour (0xff071a1a), 0, 0,
	                            juce::Colour (0xff0a1030), 0, (float) getHeight(), false);
	g.setGradientFill (grad);
	g.fillAll();

	// Title
	g.setColour (juce::Colour (0xff66ffdd));
	g.setFont (juce::Font (20.0f, juce::Font::bold));
	g.drawFittedText ("BoDSP Filter",
	                  getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Separator
	g.setColour (juce::Colour (0x3366ffdd));
	g.fillRect (20, 38, getWidth() - 40, 1);

	// Label above mode box
	g.setColour (juce::Colours::white.withAlpha (0.7f));
	g.setFont (12.0f);
	g.drawText ("Mode", 20, 44, 120, 16, juce::Justification::centredLeft, false);
}

void BoDSPFilterAudioProcessorEditor::resized()
{
	// Mode box across the top-left
	modeBox.setBounds (20, 62, 160, 26);

	// Four knobs in a row to the right and below
	const int knobW = 90;
	const int knobH = 100;
	const int labelH = 18;
	const int knobY = 48;
	const int startX = 210;

	auto placeKnob = [&] (juce::Slider& s, juce::Label& lbl, int col)
	{
		const int x = startX + col * knobW;
		lbl.setBounds (x, knobY, knobW, labelH);
		s.setBounds (x, knobY + labelH, knobW, knobH);
	};

	placeKnob (freqSlider, freqLabel, 0);
	placeKnob (qSlider,    qLabel,    1);
	placeKnob (gainSlider, gainLabel, 2);
	placeKnob (mixSlider,  mixLabel,  3);

	// Meter at the bottom
	meter.setBounds (20, knobY + labelH + knobH + 16, getWidth() - 40, 22);
}

void BoDSPFilterAudioProcessorEditor::timerCallback()
{
	meter.setLevel (processorRef.getOutputMeter());
}
