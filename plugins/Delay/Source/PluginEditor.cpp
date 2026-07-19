#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
static void setupTechnoKnob (juce::Slider& s, juce::Label& lbl, const juce::String& text, juce::Component* parent)
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
BoDSPDelayAudioProcessorEditor::BoDSPDelayAudioProcessorEditor (BoDSPDelayAudioProcessor& p)
	: juce::AudioProcessorEditor (&p), processorRef (p)
{
	setResizable (true, true);
	setResizeLimits (420, 220, 1000, 600);
	setSize (520, 280);

	auto& apvts = processorRef.apvts;

	// Knobs
	setupTechnoKnob (timeSlider,     timeLabel,     "Time",     this);
	timeSlider.setTextValueSuffix (" ms");
	timeAttach = std::make_unique<Attach> (apvts, "timeMs", timeSlider);

	setupTechnoKnob (feedbackSlider, feedbackLabel, "Feedback", this);
	feedbackAttach = std::make_unique<Attach> (apvts, "feedback", feedbackSlider);

	setupTechnoKnob (mixSlider,      mixLabel,      "Mix",      this);
	mixAttach = std::make_unique<Attach> (apvts, "mix", mixSlider);

	setupTechnoKnob (lpSlider,       lpLabel,       "LP Cut",   this);
	lpSlider.setTextValueSuffix (" Hz");
	lpSlider.setSkewFactorFromMidPoint (1500.0);
	lpAttach = std::make_unique<Attach> (apvts, "lp", lpSlider);

	setupTechnoKnob (hpSlider,       hpLabel,       "HP Cut",   this);
	hpSlider.setTextValueSuffix (" Hz");
	hpSlider.setSkewFactorFromMidPoint (150.0);
	hpAttach = std::make_unique<Attach> (apvts, "hp", hpSlider);

	// Toggles
	setupTechnoToggle (duckToggle, this);
	duckAttach = std::make_unique<ButtonAttach> (apvts, "duckEnable", duckToggle);

	setupTechnoToggle (clipToggle, this);
	clipAttach = std::make_unique<ButtonAttach> (apvts, "softClip", clipToggle);

	addAndMakeVisible (meter);
	startTimerHz (30);
}

BoDSPDelayAudioProcessorEditor::~BoDSPDelayAudioProcessorEditor() {}

//==============================================================================
void BoDSPDelayAudioProcessorEditor::paint (juce::Graphics& g)
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
	g.drawFittedText ("BoDSP Delay", getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Subtle divider line
	g.setColour (juce::Colour (0x33ff3300));
	g.fillRect (20, 38, getWidth() - 40, 1);
}

void BoDSPDelayAudioProcessorEditor::resized()
{
	const int w = getWidth();
	const int h = getHeight();

	// Toggles on the left
	const int sidebarW = juce::jlimit (120, 150, (int) (w * 0.25f));
	duckToggle.setBounds (20, 52, sidebarW, 26);
	clipToggle.setBounds (20, 88, sidebarW, 26);

	// Knobs on the right
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

	placeKnob (timeSlider,     timeLabel,     0);
	placeKnob (feedbackSlider, feedbackLabel, 1);
	placeKnob (mixSlider,      mixLabel,      2);
	placeKnob (lpSlider,       lpLabel,       3);
	placeKnob (hpSlider,       hpLabel,       4);

	// Meter at the bottom
	meter.setBounds (20, h - 42, w - 40, 22);
}

void BoDSPDelayAudioProcessorEditor::timerCallback()
{
	meter.setLevel (processorRef.getOutputMeter());
}
