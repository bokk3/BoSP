#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
BoDSPChorusAudioProcessorEditor::BoDSPChorusAudioProcessorEditor (BoDSPChorusAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      vuMeter (p),
      rateAtt     (p.apvts, "rate",     rateKnob),
      depthAtt    (p.apvts, "depth",    depthKnob),
      mixAtt      (p.apvts, "mix",      mixKnob),
      feedbackAtt (p.apvts, "feedback", feedbackKnob),
      spreadAtt   (p.apvts, "spread",   spreadKnob),
      clipperAtt  (p.apvts, "softClip", clipperToggle)
{
    // Setup each knob + label
    setupKnob (rateKnob,     rateLabel,     "RATE");
    setupKnob (depthKnob,    depthLabel,    "DEPTH");
    setupKnob (mixKnob,      mixLabel,      "MIX");
    setupKnob (feedbackKnob, feedbackLabel, "FEEDBACK");
    setupKnob (spreadKnob,   spreadLabel,   "SPREAD");

    // Clipper toggle
    clipperToggle.setColour (juce::ToggleButton::tickColourId,      juce::Colour (0xffff3300));
    clipperToggle.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff555566));
    clipperToggle.setColour (juce::ToggleButton::textColourId,      juce::Colour (0xffccccdd));
    addAndMakeVisible (clipperToggle);

    addAndMakeVisible (vuMeter);

    setSize (560, 260);
    setResizable (true, true);
    setResizeLimits (480, 220, 800, 380);
}

BoDSPChorusAudioProcessorEditor::~BoDSPChorusAudioProcessorEditor() {}

//==============================================================================
void BoDSPChorusAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& l,
                                                  const juce::String& text)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    s.setColour (juce::Slider::rotarySliderFillColourId,   juce::Colour (0xffff3300));
    s.setColour (juce::Slider::rotarySliderOutlineColourId,juce::Colour (0xff2a2535));
    s.setColour (juce::Slider::thumbColourId,              juce::Colour (0xffff6622));
    s.setColour (juce::Slider::textBoxTextColourId,        juce::Colour (0xffccccdd));
    s.setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colour (0xff0d0b14));
    s.setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    addAndMakeVisible (s);

    l.setText (text, juce::dontSendNotification);
    l.setFont (juce::Font (11.0f, juce::Font::bold));
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, juce::Colour (0xffff3300));
    addAndMakeVisible (l);
}

//==============================================================================
void BoDSPChorusAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Dark industrial gradient background
    juce::ColourGradient bg (juce::Colour (0xff07070a), bounds.getTopLeft(),
                             juce::Colour (0xff14121d), bounds.getBottomRight(), false);
    bg.addColour (0.5, juce::Colour (0xff0f0d18));
    g.setGradientFill (bg);
    g.fillAll();

    // Neon accent header bar
    g.setColour (juce::Colour (0xffff3300));
    g.fillRect (0, 0, getWidth(), 4);

    // Plugin title
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xffff3300));
    g.drawText ("BoDSP  CHORUS", 16, 10, 260, 32, juce::Justification::left, false);

    // Subtle subtitle
    g.setFont (juce::Font (10.5f));
    g.setColour (juce::Colour (0xff665577));
    g.drawText ("4-VOICE STEREO MODULATOR", 16, 36, 260, 16, juce::Justification::left, false);

    // Separator line
    g.setColour (juce::Colour (0xff2a2535));
    g.drawHorizontalLine (56, 0.0f, static_cast<float> (getWidth()));
}

//==============================================================================
void BoDSPChorusAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Reserve right column for VU meter
    const int meterW  = 24;
    const int meterX  = w - meterW - 12;
    const int meterY  = 64;
    const int meterH  = h - meterY - 12;
    vuMeter.setBounds (meterX, meterY, meterW, meterH);

    // Available width for knobs
    const int usableW = meterX - 8;

    // Clipper toggle — top right inside usable area
    const int toggleW = 72;
    const int toggleH = 22;
    clipperToggle.setBounds (usableW - toggleW, 62, toggleW, toggleH);

    // Knob layout: 5 knobs across the usable width
    const int knobAreaY = 60;
    const int knobAreaH = h - knobAreaY - 8;
    const int numKnobs  = 5;
    const int knobW     = (usableW - toggleW - 8) / numKnobs;
    const int knobH     = std::min (knobAreaH, 130);
    const int labelH    = 16;

    auto placeKnob = [&](juce::Slider& s, juce::Label& l, int idx)
    {
        const int x = idx * knobW;
        const int y = knobAreaY + (knobAreaH - knobH - labelH) / 2;
        l.setBounds (x, y, knobW, labelH);
        s.setBounds (x, y + labelH, knobW, knobH);
    };

    placeKnob (rateKnob,     rateLabel,     0);
    placeKnob (depthKnob,    depthLabel,    1);
    placeKnob (mixKnob,      mixLabel,      2);
    placeKnob (feedbackKnob, feedbackLabel, 3);
    placeKnob (spreadKnob,   spreadLabel,   4);
}
