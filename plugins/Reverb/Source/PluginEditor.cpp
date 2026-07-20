#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupTechnoKnob (juce::Slider& s, juce::Label& lbl, const juce::String& text,
                              juce::Component* parent)
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
BoDSPReverbAudioProcessorEditor::BoDSPReverbAudioProcessorEditor (BoDSPReverbAudioProcessor& p)
	: AudioProcessorEditor (&p), processorRef (p)
{
	setResizable (true, true);
	setResizeLimits (500, 220, 1200, 600);
	setSize (560, 280);

	auto& apvts = processorRef.apvts;

	setupTechnoKnob (roomSizeSlider, roomSizeLabel, "Room Size",  this);
	setupTechnoKnob (dampingSlider,  dampingLabel,  "Damping",    this);
	setupTechnoKnob (widthSlider,    widthLabel,    "Width",      this);
	setupTechnoKnob (mixSlider,      mixLabel,      "Mix",        this);
	setupTechnoKnob (preDelaySlider, preDelayLabel, "Pre-Delay",  this);
	setupTechnoKnob (lpSlider,       lpLabel,       "LP Cut",     this);
	setupTechnoKnob (hpSlider,       hpLabel,       "HP Cut",     this);
	setupTechnoKnob (outputGainSlider, outputGainLabel, "Output", this);

	preDelaySlider.setTextValueSuffix (" ms");
	lpSlider.setTextValueSuffix (" Hz");
	hpSlider.setTextValueSuffix (" Hz");
	outputGainSlider.setTextValueSuffix (" dB");
	lpSlider.setSkewFactorFromMidPoint (5000.0);
	hpSlider.setSkewFactorFromMidPoint (100.0);

	roomSizeAttach  = std::make_unique<Attach> (apvts, "roomSize",   roomSizeSlider);
	dampingAttach   = std::make_unique<Attach> (apvts, "damping",    dampingSlider);
	widthAttach     = std::make_unique<Attach> (apvts, "width",      widthSlider);
	mixAttach       = std::make_unique<Attach> (apvts, "mix",        mixSlider);
	preDelayAttach  = std::make_unique<Attach> (apvts, "preDelay",   preDelaySlider);
	lpAttach        = std::make_unique<Attach> (apvts, "lpCutoff",   lpSlider);
	hpAttach        = std::make_unique<Attach> (apvts, "hpCutoff",   hpSlider);
	outputGainAttach = std::make_unique<Attach> (apvts, "outputGain", outputGainSlider);

	// Clipper Toggle
	setupTechnoToggle (clipToggle, this);
	clipAttach = std::make_unique<ButtonAttach> (apvts, "softClip", clipToggle);

	addAndMakeVisible (meter);
	startTimerHz (30);
}

BoDSPReverbAudioProcessorEditor::~BoDSPReverbAudioProcessorEditor() {}

//==============================================================================
void BoDSPReverbAudioProcessorEditor::paint (juce::Graphics& g)
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
	g.drawFittedText ("BoDSP Reverb", getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Subtle divider line
	g.setColour (juce::Colour (0x33ff3300));
	g.fillRect (20, 38, getWidth() - 40, 1);
}

void BoDSPReverbAudioProcessorEditor::resized()
{
	const int w = getWidth();
	const int h = getHeight();

	// 8 knobs — widen slightly to fit
	const int knobW = (w - 40) / 8;
	const int knobH = juce::jmin (100, h - 120);
	const int labelH = 18;
	const int rowY = 48;
	const int startX = 20;

	auto placeKnob = [&] (juce::Slider& s, juce::Label& lbl, int col)
	{
		const int x = startX + col * knobW;
		lbl.setBounds (x, rowY, knobW, labelH);
		s.setBounds (x, rowY + labelH, knobW, knobH);
	};

	placeKnob (roomSizeSlider,   roomSizeLabel,   0);
	placeKnob (dampingSlider,    dampingLabel,    1);
	placeKnob (widthSlider,      widthLabel,      2);
	placeKnob (mixSlider,        mixLabel,        3);
	placeKnob (preDelaySlider,   preDelayLabel,   4);
	placeKnob (lpSlider,         lpLabel,         5);
	placeKnob (hpSlider,         hpLabel,         6);
	placeKnob (outputGainSlider, outputGainLabel, 7);

	// Clipper placed next to title / top right header area
	clipToggle.setBounds (w - 110, 8, 90, 24);

	// Meter at the bottom
	meter.setBounds (20, h - 42, w - 40, 22);
}

void BoDSPReverbAudioProcessorEditor::timerCallback()
{
	meter.setLevel (processorRef.getOutputMeter());
}
