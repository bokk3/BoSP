#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupTechnoKnob (juce::Slider& s, juce::Label& lbl,
                             const juce::String& text, juce::Component* parent)
{
	s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
	s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
	s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff3300));    // Neon Orange/Red
	s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff1f1f26)); // Dark metal
	s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));               // White indicator line
	s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
	s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
	parent->addAndMakeVisible (s);

	lbl.setText (text, juce::dontSendNotification);
	lbl.setJustificationType (juce::Justification::centred);
	lbl.setFont (juce::Font (12.0f, juce::Font::bold));
	lbl.setColour (juce::Label::textColourId, juce::Colour (0xffa0a0ab));
	parent->addAndMakeVisible (lbl);
}

static void setupTechnoToggle (juce::ToggleButton& b, juce::Component* parent)
{
	b.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffff3300));
	b.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
	parent->addAndMakeVisible (b);
}

//==============================================================================
BoDSPFilterAudioProcessorEditor::BoDSPFilterAudioProcessorEditor (BoDSPFilterAudioProcessor& p)
	: AudioProcessorEditor (&p), processorRef (p)
{
	setResizable (true, true);
	setResizeLimits (420, 220, 1000, 600);
	setSize (520, 260);

	auto& apvts = processorRef.apvts;

	// Mode combo box
	modeBox.addItem ("Lowpass",    1);
	modeBox.addItem ("Highpass",   2);
	modeBox.addItem ("Low Shelf",  3);
	modeBox.addItem ("High Shelf", 4);
	modeBox.addItem ("Notch",      5);
	addAndMakeVisible (modeBox);
	modeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff121216));
	modeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff33333d));
	modeBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
	modeBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffff3300));
	modeAttach = std::make_unique<ModeAttach> (apvts, "mode", modeBox);

	// Knobs
	setupTechnoKnob (freqSlider, freqLabel, "Frequency", this);
	freqSlider.setTextValueSuffix (" Hz");
	freqSlider.setSkewFactorFromMidPoint (1000.0);
	freqAttach = std::make_unique<Attach> (apvts, "freq", freqSlider);

	setupTechnoKnob (qSlider, qLabel, "Q", this);
	qAttach = std::make_unique<Attach> (apvts, "q", qSlider);

	setupTechnoKnob (gainSlider, gainLabel, "Gain", this);
	gainSlider.setTextValueSuffix (" dB");
	gainAttach = std::make_unique<Attach> (apvts, "gainDb", gainSlider);

	setupTechnoKnob (mixSlider, mixLabel, "Mix", this);
	mixAttach = std::make_unique<Attach> (apvts, "mix", mixSlider);

	setupTechnoKnob (outputGainSlider, outputGainLabel, "Output", this);
	outputGainSlider.setTextValueSuffix (" dB");
	outputGainAttach = std::make_unique<Attach> (apvts, "outputGain", outputGainSlider);


	// Clipper Toggle
	setupTechnoToggle (clipToggle, this);
	clipAttach = std::make_unique<ButtonAttach> (apvts, "softClip", clipToggle);

	addAndMakeVisible (meter);
	startTimerHz (30);
}

BoDSPFilterAudioProcessorEditor::~BoDSPFilterAudioProcessorEditor() {}

//==============================================================================
void BoDSPFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
	// Hard Techno Gradient Background (hardware vibe)
	juce::ColourGradient grad (juce::Colour (0xff07070a), 0, 0,
	                            juce::Colour (0xff14121d), 0, (float) getHeight(), false);
	g.setGradientFill (grad);
	g.fillAll();

	// Accent border lines (Neon red TR-909 theme)
	g.setColour (juce::Colour (0x22ff3300));
	g.drawRect (getLocalBounds(), 4.0f);

	// Title
	g.setColour (juce::Colour (0xffff3300));
	g.setFont (juce::Font (20.0f, juce::Font::bold));
	g.drawFittedText ("BoDSP Filter", getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Subtle divider line
	g.setColour (juce::Colour (0x33ff3300));
	g.fillRect (20, 38, getWidth() - 40, 1);

	// Label above mode box
	g.setColour (juce::Colours::white.withAlpha (0.7f));
	g.setFont (juce::Font (12.0f, juce::Font::bold));
	g.drawText ("Filter Mode", 20, 44, 120, 16, juce::Justification::centredLeft, false);
}

void BoDSPFilterAudioProcessorEditor::resized()
{
	const int w = getWidth();
	const int h = getHeight();

	// Mode box & Clipper toggle on the left, scaling with sidebar width
	const int sidebarW = juce::jlimit (120, 180, (int) (w * 0.3f));
	modeBox.setBounds (20, 62, sidebarW, 26);
	clipToggle.setBounds (20, 98, sidebarW, 26);

	// Knobs in the remaining space
	const int startX = 20 + sidebarW + 20;
	const int contentW = w - startX - 20;
	const int knobW = contentW / 5;
	const int knobH = juce::jmin (100, h - 120);
	const int labelH = 18;
	const int knobY = 48;

	auto placeKnob = [&] (juce::Slider& s, juce::Label& lbl, int col)
	{
		const int x = startX + col * knobW;
		lbl.setBounds (x, knobY, knobW, labelH);
		s.setBounds (x, knobY + labelH, knobW, knobH);
	};

	placeKnob (freqSlider,       freqLabel,       0);
	placeKnob (qSlider,          qLabel,          1);
	placeKnob (gainSlider,       gainLabel,       2);
	placeKnob (mixSlider,        mixLabel,        3);
	placeKnob (outputGainSlider, outputGainLabel, 4);

	// Meter at the bottom
	meter.setBounds (20, h - 42, w - 40, 22);
}

void BoDSPFilterAudioProcessorEditor::timerCallback()
{
	meter.setLevel (processorRef.getOutputMeter());
}
