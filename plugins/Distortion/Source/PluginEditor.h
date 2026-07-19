#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <atomic>
#include <cmath>

//==============================================================================
class BoDSPDistortionAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                 private juce::Timer
{
public:
    explicit BoDSPDistortionAudioProcessorEditor (BoDSPDistortionAudioProcessor&);
    ~BoDSPDistortionAudioProcessorEditor() override;

    void timerCallback() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BoDSPDistortionAudioProcessor& processorRef;

    // UI controls
    juce::Slider driveSlider;
    using DriveAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<DriveAttachment> driveAttachment;

    juce::Slider outputSlider;
    using OutputAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<OutputAttachment> outputAttachment;

    juce::ComboBox modeBox;
    using ModeAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ModeAttachment> modeAttachment;

    juce::ToggleButton softClipToggle { "Soft clip @ 0 dB" };
    using SoftClipAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SoftClipAttachment> softClipAttachment;

    juce::Slider mixSlider;
    using MixAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<MixAttachment> mixAttachment;

    juce::Slider toneSlider;
    using ToneAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ToneAttachment> toneAttachment;

    // Output meter component
    class MeterComponent : public juce::Component
    {
    public:
        MeterComponent() = default;

        void setLevel (float v) noexcept
        {
            level = v;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            g.fillAll (juce::Colours::darkgrey.darker (0.4f));

            // convert linear level to dB
            const float lin = std::max (level, 1e-9f);
            const float db = 20.0f * std::log10 (lin);
            const float minDb = -60.0f;
            const float maxDb = 6.0f;
            float frac = (db - minDb) / (maxDb - minDb);
            frac = juce::jlimit (0.0f, 1.0f, frac);

            const float fillW = bounds.getWidth() * frac;

            juce::Colour fillCol;
            if (db > 0.0f) fillCol = juce::Colours::purple;
            else if (db > -3.0f) fillCol = juce::Colours::red;
            else if (db > -6.0f) fillCol = juce::Colours::yellow;
            else fillCol = juce::Colours::green;

            g.setColour (fillCol);
            g.fillRect (bounds.withWidth (fillW));

            // draw border and db text
            g.setColour (juce::Colours::black);
            g.drawRect (bounds);
            g.setColour (juce::Colours::white);
            g.setFont (12.0f);
            g.drawText (juce::String (db, 1) + " dB", getLocalBounds(), juce::Justification::centredRight, false);
        }

    private:
        float level { 0.0f };
    } meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoDSPDistortionAudioProcessorEditor)
};
