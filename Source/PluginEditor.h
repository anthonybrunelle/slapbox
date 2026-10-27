#pragma once

#include "PluginProcessor.h"

// The embedded Poppins typefaces, shared by everything that draws text.
struct EmbeddedFonts
{
    EmbeddedFonts();
    juce::Typeface::Ptr regular, semibold;
};

// Recessed dark knobs with a thin glowing arc and a dot pointer,
// plus a glowing two-thumb filter slider.
class SlapboxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 7; }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;

    // Value bubble shown while hovering or dragging a knob
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip,
                     const juce::Rectangle<float>& body) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;
    int getSliderPopupPlacement (juce::Slider& s) override
    {
        return s.getProperties()["bubbleLeft"] ? juce::BubbleComponent::left : juce::BubbleComponent::above;
    }
};

class SlapboxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SlapboxAudioProcessorEditor (SlapboxAudioProcessor&);
    ~SlapboxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label name;
        std::unique_ptr<Attachment> attachment;
    };

    void updateFilterLabels();

    SlapboxAudioProcessor& proc;
    juce::SharedResourcePointer<EmbeddedFonts> fonts;   // keeps the typefaces alive while the editor is open
    SlapboxLookAndFeel lnf;
    // 0-2: big knobs (time, feedback, mix); 3-4: small stacked knobs (width, saturation)
    std::array<Knob, 5> knobs;
    juce::Line<float> stackDivider;
    int contentLeft = 0;   // left edge of the content column; the logo lines up with it

    // Low cut = left thumb, high cut = right thumb. SliderAttachment can't drive a
    // two-value slider, so each thumb gets its own ParameterAttachment.
    juce::Slider filter { juce::Slider::TwoValueHorizontal, juce::Slider::NoTextBox };
    juce::Label hpfValue, lpfValue;
    std::unique_ptr<juce::ParameterAttachment> hpfAttachment, lpfAttachment;
    juce::Rectangle<float> hpfIcon, lpfIcon;
    float dividerY = 0.0f;

    juce::TextButton bypass { "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapboxAudioProcessorEditor)
};
