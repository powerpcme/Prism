#include "LookAndFeel.h"

const juce::Colour OualLookAndFeel::backgroundColour = juce::Colour(0xff2a2a2a);
const juce::Colour OualLookAndFeel::panelColour = juce::Colour(0xff3a3a3a);
const juce::Colour OualLookAndFeel::accentColour = juce::Colour(0xffffffff);
const juce::Colour OualLookAndFeel::textColour = juce::Colour(0xffffffff);
const juce::Colour OualLookAndFeel::activeVoiceColour = juce::Colour(0xffffffff);
const juce::Colour OualLookAndFeel::inactiveVoiceColour = juce::Colour(0xff606060);
const juce::Colour OualLookAndFeel::sliderTrackColour = juce::Colour(0xff505050);
const juce::Colour OualLookAndFeel::sliderThumbColour = juce::Colour(0xffffffff);

OualLookAndFeel::OualLookAndFeel()
{
    // Set default colours
    setColour(juce::ResizableWindow::backgroundColourId, backgroundColour);
    setColour(juce::Slider::backgroundColourId, sliderTrackColour);
    setColour(juce::Slider::thumbColourId, sliderThumbColour);
    setColour(juce::Slider::trackColourId, accentColour);
    setColour(juce::Label::textColourId, textColour);
    setColour(juce::ComboBox::backgroundColourId, panelColour);
    setColour(juce::ComboBox::textColourId, textColour);
    setColour(juce::ComboBox::outlineColourId, accentColour);
    setColour(juce::PopupMenu::backgroundColourId, panelColour);
    setColour(juce::PopupMenu::textColourId, textColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour);
    setColour(juce::TextButton::buttonColourId, panelColour);
    setColour(juce::TextButton::textColourOffId, textColour);
}

void OualLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                        const juce::Slider::SliderStyle /*style*/,
                                        juce::Slider& slider)
{
    auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? static_cast<float>(height) * 0.25f
                                                              : static_cast<float>(width) * 0.25f);

    juce::Point<float> startPoint(slider.isHorizontal() ? static_cast<float>(x) : static_cast<float>(x) + static_cast<float>(width) * 0.5f,
                                  slider.isHorizontal() ? static_cast<float>(y) + static_cast<float>(height) * 0.5f : static_cast<float>(height + y));

    juce::Point<float> endPoint(slider.isHorizontal() ? static_cast<float>(width + x) : startPoint.x,
                                slider.isHorizontal() ? startPoint.y : static_cast<float>(y));

    // Draw track background
    g.setColour(sliderTrackColour);
    g.drawLine(juce::Line<float>(startPoint, endPoint), trackWidth);

    // Draw filled portion
    juce::Point<float> thumbPoint(slider.isHorizontal() ? sliderPos : startPoint.x,
                                  slider.isHorizontal() ? startPoint.y : sliderPos);

    g.setColour(accentColour);
    g.drawLine(juce::Line<float>(startPoint, thumbPoint), trackWidth);

    // Draw thumb
    float thumbSize = trackWidth * 2.5f;
    g.setColour(sliderThumbColour);
    g.fillEllipse(juce::Rectangle<float>(thumbSize, thumbSize).withCentre(thumbPoint));

    // Draw thumb outline
    g.setColour(textColour);
    g.drawEllipse(juce::Rectangle<float>(thumbSize, thumbSize).withCentre(thumbPoint), 1.5f);
}

void OualLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& /*slider*/)
{
    auto radius = static_cast<float>(juce::jmin(width / 2, height / 2)) - 4.0f;
    auto centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    auto centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Fill
    g.setColour(panelColour);
    g.fillEllipse(rx, ry, rw, rw);

    // Outline
    g.setColour(sliderTrackColour);
    g.drawEllipse(rx, ry, rw, rw, 3.0f);

    // Arc (value indicator)
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                          0.0f, rotaryStartAngle, angle, true);
    g.setColour(accentColour);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f));

    // Pointer
    juce::Path p;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 3.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius + 4.0f, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(textColour);
    g.fillPath(p);
}

void OualLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f, 0.5f);

    g.setColour(panelColour);
    g.fillRect(bounds);

    g.setColour(box.isEnabled() ? accentColour : sliderTrackColour);
    g.drawRect(bounds, 1.5f);

    // Arrow
    auto arrowZone = juce::Rectangle<int>(width - 20, 0, 15, height);
    juce::Path arrow;
    arrow.addTriangle(static_cast<float>(arrowZone.getX()) + 3.0f,
                      static_cast<float>(arrowZone.getCentreY()) - 2.0f,
                      static_cast<float>(arrowZone.getCentreX()),
                      static_cast<float>(arrowZone.getCentreY()) + 3.0f,
                      static_cast<float>(arrowZone.getRight()) - 3.0f,
                      static_cast<float>(arrowZone.getCentreY()) - 2.0f);
    g.setColour(textColour);
    g.fillPath(arrow);
}

void OualLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4.0f;
    auto r = bounds.withSizeKeepingCentre(size, size);

    // Background
    g.setColour(panelColour);
    g.fillEllipse(r);

    // Border
    g.setColour(shouldDrawButtonAsHighlighted ? textColour : sliderTrackColour);
    g.drawEllipse(r, 2.0f);

    // Active indicator
    if (button.getToggleState())
    {
        g.setColour(activeVoiceColour);
        g.fillEllipse(r.reduced(size * 0.25f));
    }
}

void OualLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

        g.setColour(textColour);
        g.setFont(getLabelFont(label));
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                         juce::jmax(1, static_cast<int>((static_cast<float>(textArea.getHeight()) / g.getCurrentFont().getHeight()))),
                         label.getMinimumHorizontalScale());
    }
}

juce::Font OualLookAndFeel::getLabelFont(juce::Label& /*label*/)
{
    return juce::Font(14.0f);
}

juce::Font OualLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return juce::Font(14.0f);
}
