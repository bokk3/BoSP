#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupKnobStatic (juce::Slider& s, juce::Label& lbl, const juce::String& text,
                              juce::Component* parent)
{
	s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
	s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
	parent->addAndMakeVisible (s);

	lbl.setText (text, juce::dontSendNotification);
	lbl.setJustificationType (juce::Justification::centred);
	lbl.setFont (juce::Font (12.0f));
	lbl.setColour (juce::Label::textColourId, juce::Colours::white);
	parent->addAndMakeVisible (lbl);
}

//==============================================================================
BoDSPReverbAudioProcessorEditor::BoDSPReverbAudioProcessorEditor (BoDSPReverbAudioProcessor& p)
	: AudioProcessorEditor (&p), processorRef (p)
{
	setSize (560, 280);

	auto& apvts = processorRef.apvts;

	setupKnobStatic (roomSizeSlider, roomSizeLabel, "Room Size",  this);
	setupKnobStatic (dampingSlider,  dampingLabel,  "Damping",    this);
	setupKnobStatic (widthSlider,    widthLabel,    "Width",      this);
	setupKnobStatic (mixSlider,      mixLabel,      "Mix",        this);
	setupKnobStatic (preDelaySlider, preDelayLabel, "Pre-Delay",  this);
	setupKnobStatic (lpSlider,       lpLabel,       "LP Cutoff",  this);
	setupKnobStatic (hpSlider,       hpLabel,       "HP Cutoff",  this);

	preDelaySlider.setTextValueSuffix (" ms");
	lpSlider.setTextValueSuffix (" Hz");
	hpSlider.setTextValueSuffix (" Hz");
	lpSlider.setSkewFactorFromMidPoint (5000.0);
	hpSlider.setSkewFactorFromMidPoint (100.0);

	roomSizeAttach = std::make_unique<Attach> (apvts, "roomSize",  roomSizeSlider);
	dampingAttach  = std::make_unique<Attach> (apvts, "damping",   dampingSlider);
	widthAttach    = std::make_unique<Attach> (apvts, "width",     widthSlider);
	mixAttach      = std::make_unique<Attach> (apvts, "mix",       mixSlider);
	preDelayAttach = std::make_unique<Attach> (apvts, "preDelay",  preDelaySlider);
	lpAttach       = std::make_unique<Attach> (apvts, "lpCutoff",  lpSlider);
	hpAttach       = std::make_unique<Attach> (apvts, "hpCutoff",  hpSlider);

	addAndMakeVisible (meter);
	startTimerHz (30);
}

BoDSPReverbAudioProcessorEditor::~BoDSPReverbAudioProcessorEditor() {}

//==============================================================================
void BoDSPReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
	// Dark gradient background
	juce::ColourGradient grad (juce::Colour (0xff0d0d1a), 0, 0,
	                            juce::Colour (0xff1a1040), 0, (float) getHeight(), false);
	g.setGradientFill (grad);
	g.fillAll();

	// Title
	g.setColour (juce::Colour (0xffb0aaff));
	g.setFont (juce::Font (20.0f, juce::Font::bold));
	g.drawFittedText ("BoDSP Reverb",
	                  getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Subtle separator below title
	g.setColour (juce::Colour (0x33b0aaff));
	g.fillRect (20, 38, getWidth() - 40, 1);
}

void BoDSPReverbAudioProcessorEditor::resized()
{
	// Seven knobs in a row: columns of 80px each, centred in 560px
	const int knobW = 80;
	const int knobH = 100;
	const int labelH = 18;
	const int rowY = 48;
	const int startX = 0;

	auto placeKnob = [&] (juce::Slider& s, juce::Label& lbl, int col)
	{
		const int x = startX + col * knobW;
		lbl.setBounds (x, rowY, knobW, labelH);
		s.setBounds (x, rowY + labelH, knobW, knobH);
	};

	placeKnob (roomSizeSlider, roomSizeLabel, 0);
	placeKnob (dampingSlider,  dampingLabel,  1);
	placeKnob (widthSlider,    widthLabel,    2);
	placeKnob (mixSlider,      mixLabel,      3);
	placeKnob (preDelaySlider, preDelayLabel, 4);
	placeKnob (lpSlider,       lpLabel,       5);
	placeKnob (hpSlider,       hpLabel,       6);

	meter.setBounds (20, rowY + labelH + knobH + 16, getWidth() - 40, 22);
}

void BoDSPReverbAudioProcessorEditor::timerCallback()
{
	meter.setLevel (processorRef.getOutputMeter());
}
