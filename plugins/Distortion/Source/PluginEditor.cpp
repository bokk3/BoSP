#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupTechnoKnob (juce::Slider& s, const juce::String& suffix, juce::Component* parent)
{
	s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
	s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
	s.setTextValueSuffix (suffix);
	s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff3300));    // Neon Orange/Red
	s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff1f1f26)); // Dark metal
	s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));               // White indicator line
	s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
	s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
	parent->addAndMakeVisible (s);
}

static void setupTechnoToggle (juce::ToggleButton& b, juce::Component* parent)
{
	b.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffff3300));
	b.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
	parent->addAndMakeVisible (b);
}

//==============================================================================
BoDSPDistortionAudioProcessorEditor::BoDSPDistortionAudioProcessorEditor (BoDSPDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setResizable (true, true);
    setResizeLimits (420, 260, 1000, 600);
    setSize (520, 300);

    // Drive slider
    setupTechnoKnob (driveSlider, " dB", this);
    driveSlider.setRange (0.0, 24.0, 0.01);
    driveAttachment = std::make_unique<DriveAttachment> (processorRef.apvts, "drive", driveSlider);

    // Output slider
    setupTechnoKnob (outputSlider, " dB", this);
    outputSlider.setRange (-24.0, 24.0, 0.01);
    outputAttachment = std::make_unique<OutputAttachment> (processorRef.apvts, "output", outputSlider);

    // Mix slider (dry/wet)
    setupTechnoKnob (mixSlider, " %", this);
    mixSlider.setRange (0.0, 100.0, 0.1);
    mixAttachment = std::make_unique<MixAttachment> (processorRef.apvts, "mix", mixSlider);

    // Tone slider
    setupTechnoKnob (toneSlider, " Hz", this);
    toneSlider.setRange (20.0, 20000.0, 1.0);
    toneSlider.setSkewFactorFromMidPoint (1000.0);
    toneAttachment = std::make_unique<ToneAttachment> (processorRef.apvts, "tone", toneSlider);

    // Mode combo box
    modeBox.addItem ("Soft", 1);
    modeBox.addItem ("Tube", 2);
    modeBox.addItem ("Hard", 3);
    modeBox.addItem ("Fold", 4);
    addAndMakeVisible (modeBox);
    modeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff121216));
    modeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff33333d));
    modeBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    modeBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffff3300));
    modeAttachment = std::make_unique<ModeAttachment> (processorRef.apvts, "mode", modeBox);

    // Soft clip toggle
    setupTechnoToggle (softClipToggle, this);
    softClipAttachment = std::make_unique<SoftClipAttachment> (processorRef.apvts, "softClip0db", softClipToggle);

    // Meter
    addAndMakeVisible (meter);

    startTimerHz (30);
}

BoDSPDistortionAudioProcessorEditor::~BoDSPDistortionAudioProcessorEditor() {}

//==============================================================================
void BoDSPDistortionAudioProcessorEditor::paint (juce::Graphics& g)
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
	g.drawFittedText ("BoDSP Distortion", getLocalBounds().withHeight (36).withY (6),
	                  juce::Justification::centred, 1);

	// Subtle divider line
	g.setColour (juce::Colour (0x33ff3300));
	g.fillRect (20, 38, getWidth() - 40, 1);

	// Labels for knobs
	g.setColour (juce::Colour (0xffa0a0ab));
	g.setFont (juce::Font (12.0f, juce::Font::bold));
	const int w = getWidth();
	const int knobW = (w - 40) / 4;
	g.drawText ("Drive",    20 + 0 * knobW, 46, knobW, 16, juce::Justification::centred, false);
	g.drawText ("Output",   20 + 1 * knobW, 46, knobW, 16, juce::Justification::centred, false);
	g.drawText ("Mix",      20 + 2 * knobW, 46, knobW, 16, juce::Justification::centred, false);
	g.drawText ("Tone",     20 + 3 * knobW, 46, knobW, 16, juce::Justification::centred, false);

	// Sidebar/bottom labels
	g.setColour (juce::Colours::white.withAlpha (0.7f));
	g.setFont (12.0f);
	g.drawText ("Waveshaper Mode", 20, getHeight() - 116, 140, 16, juce::Justification::centredLeft, false);
}

void BoDSPDistortionAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // 4 knobs in a row
    const int knobW = (w - 40) / 4;
    const int knobH = juce::jmin (110, h - 160);
    const int startX = 20;

    driveSlider.setBounds (startX + 0 * knobW, 64, knobW, knobH);
    outputSlider.setBounds (startX + 1 * knobW, 64, knobW, knobH);
    mixSlider.setBounds (startX + 2 * knobW, 64, knobW, knobH);
    toneSlider.setBounds (startX + 3 * knobW, 64, knobW, knobH);

    // Mode box and toggle below the knobs
    const int controlY = h - 90;
    modeBox.setBounds (20, controlY, 140, 26);
    softClipToggle.setBounds (180, controlY, 160, 26);

    meter.setBounds (20, h - 44, w - 40, 24);
}

void BoDSPDistortionAudioProcessorEditor::timerCallback()
{
    const float v = processorRef.getOutputMeter();
    meter.setLevel (v);
}
