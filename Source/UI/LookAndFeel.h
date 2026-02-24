#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class OualLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OualLookAndFeel();

    // Colors
    static const juce::Colour backgroundColour;
    static const juce::Colour panelColour;
    static const juce::Colour accentColour;
    static const juce::Colour textColour;
    static const juce::Colour activeVoiceColour;
    static const juce::Colour inactiveVoiceColour;
    static const juce::Colour sliderTrackColour;
    static const juce::Colour sliderThumbColour;

    // Override slider drawing
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // Override combo box
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    // Override toggle button
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // Label styling
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // Font
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;

private:
    juce::Font mainFont;
};
