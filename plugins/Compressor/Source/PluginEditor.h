#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
/** Hard Techno-styled VU meter component (mirrors other BoDSP plugins). */
class CompressorMeterComponent : public juce::Component, private juce::Timer
{
public:
    explicit CompressorMeterComponent (BoDSPCompressorAudioProcessor& p) : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0d0b14));
        g.fillRoundedRectangle (bounds, 4.0f);

        const float level = std::clamp (processor.getOutputMeter(), 0.0f, 1.0f);
        const float barH  = bounds.getHeight() * level;
        const float y     = bounds.getBottom() - barH;

        // Gradient: green → orange → red
        juce::ColourGradient grad (juce::Colour (0xff00ff88), bounds.getBottomLeft(),
                                   juce::Colour (0xffff3300), bounds.getTopLeft(),
                                   false);
        grad.addColour (0.7, juce::Colour (0xffff8800));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds.getX() + 2, y,
                                bounds.getWidth() - 4, barH, 3.0f);

        g.setColour (juce::Colour (0x44ffffff));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }

    void timerCallback() override { repaint(); }

private:
    BoDSPCompressorAudioProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorMeterComponent)
};

//==============================================================================
class BoDSPCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit BoDSPCompressorAudioProcessorEditor (BoDSPCompressorAudioProcessor&);
    ~BoDSPCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BoDSPCompressorAudioProcessor& audioProcessor;

    // Knobs
    juce::Slider thresholdKnob, ratioKnob, attackKnob, releaseKnob, makeupKnob, mixKnob;
    // Labels
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel, mixLabel;
    // Soft clip toggle
    juce::ToggleButton clipperToggle { "CLIP" };
    // VU meter
    CompressorMeterComponent vuMeter;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SliderAttachment thresholdAtt, ratioAtt, attackAtt, releaseAtt, makeupAtt, mixAtt;
    ButtonAttachment clipperAtt;

    void setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPCompressorAudioProcessorEditor)
};
