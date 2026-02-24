#include "LCDDisplay.h"
#include "LookAndFeel.h"

LCDDisplay::LCDDisplay(juce::AudioProcessorValueTreeState& params, int voices)
    : parameters(params), numVoices(voices)
{
    voiceActivityLevels.fill(0.0f);
    startTimerHz(30);
}

juce::String LCDDisplay::getParamPrefix(int voiceIndex) const
{
    return "voice_" + juce::String(voiceIndex) + "_";
}

juce::Point<float> LCDDisplay::paramToXY(int voiceIndex) const
{
    auto prefix = getParamPrefix(voiceIndex);
    float pan = parameters.getRawParameterValue(prefix + "pan")->load();
    float vol = parameters.getRawParameterValue(prefix + "volume")->load();

    // X = pan (0=left, 1=right), Y = 1-volume (top=loud, bottom=quiet)
    float x = static_cast<float>(xyPadArea.getX()) + pan * static_cast<float>(xyPadArea.getWidth());
    float y = static_cast<float>(xyPadArea.getY()) + (1.0f - vol) * static_cast<float>(xyPadArea.getHeight());
    return { x, y };
}

void LCDDisplay::xyToParam(int voiceIndex, juce::Point<float> xyPos)
{
    auto prefix = getParamPrefix(voiceIndex);

    float pan = (xyPos.x - static_cast<float>(xyPadArea.getX())) / static_cast<float>(xyPadArea.getWidth());
    float vol = 1.0f - (xyPos.y - static_cast<float>(xyPadArea.getY())) / static_cast<float>(xyPadArea.getHeight());

    pan = juce::jlimit(0.0f, 1.0f, pan);
    vol = juce::jlimit(0.0f, 1.0f, vol);

    if (auto* param = parameters.getParameter(prefix + "pan"))
        param->setValueNotifyingHost(pan);
    if (auto* param = parameters.getParameter(prefix + "volume"))
        param->setValueNotifyingHost(vol);
}

int LCDDisplay::findVoiceNearPoint(juce::Point<int> point, float radius) const
{
    for (int i = 0; i < numVoices; ++i)
    {
        auto prefix = getParamPrefix(i);
        bool active = parameters.getRawParameterValue(prefix + "active")->load() > 0.5f;
        if (!active)
            continue;

        auto dotPos = paramToXY(i);
        if (point.toFloat().getDistanceFrom(dotPos) <= radius)
            return i;
    }
    return -1;
}

void LCDDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background gradient
    juce::ColourGradient gradient(OualLookAndFeel::panelColour.darker(0.3f),
                                   bounds.getTopLeft(),
                                   OualLookAndFeel::panelColour,
                                   bounds.getBottomRight(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Border
    g.setColour(OualLookAndFeel::accentColour.withAlpha(0.5f));
    g.drawRect(bounds.reduced(1.0f), 2.0f);

    // --- XY Pad Area ---
    auto padBounds = xyPadArea.toFloat();

    // Animated scan lines (subtle CRT effect)
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (float y = animationPhase + padBounds.getY(); y < padBounds.getBottom(); y += 4.0f)
    {
        g.drawHorizontalLine(static_cast<int>(y), padBounds.getX(), padBounds.getRight());
    }

    // Grid lines (subtle)
    g.setColour(OualLookAndFeel::sliderTrackColour.withAlpha(0.2f));
    int gridSize = juce::jmax(1, static_cast<int>(padBounds.getWidth()) / 8);
    for (int x = gridSize; x < static_cast<int>(padBounds.getWidth()); x += gridSize)
    {
        g.drawVerticalLine(xyPadArea.getX() + x, padBounds.getY() + 5, padBounds.getBottom() - 5);
    }
    for (int y = gridSize; y < static_cast<int>(padBounds.getHeight()); y += gridSize)
    {
        g.drawHorizontalLine(xyPadArea.getY() + y, padBounds.getX() + 5, padBounds.getRight() - 5);
    }

    // Center crosshair (pan=0.5, vol=0.5)
    g.setColour(OualLookAndFeel::sliderTrackColour.withAlpha(0.15f));
    float cx = padBounds.getX() + padBounds.getWidth() * 0.5f;
    float cy = padBounds.getY() + padBounds.getHeight() * 0.5f;
    g.drawVerticalLine(static_cast<int>(cx), padBounds.getY(), padBounds.getBottom());
    g.drawHorizontalLine(static_cast<int>(cy), padBounds.getX(), padBounds.getRight());

    // Axis labels
    g.setColour(OualLookAndFeel::textColour.withAlpha(0.4f));
    g.setFont(juce::Font(10.0f));
    g.drawText("Pan",
               juce::Rectangle<int>(xyPadArea.getCentreX() - 15, xyPadArea.getBottom() - 14, 30, 14),
               juce::Justification::centred);
    g.drawText("Vol",
               juce::Rectangle<int>(xyPadArea.getX() + 2, xyPadArea.getCentreY() - 7, 20, 14),
               juce::Justification::centredLeft);

    // Draw voice dots on the XY pad
    float dotRadius = 6.0f;
    for (int i = 0; i < numVoices; ++i)
    {
        auto prefix = getParamPrefix(i);
        bool active = parameters.getRawParameterValue(prefix + "active")->load() > 0.5f;
        if (!active)
            continue;

        auto dotPos = paramToXY(i);
        auto colour = VoiceElement::getVoiceColour(i);

        // Activity glow
        float activity = voiceActivityLevels[static_cast<size_t>(i)];
        if (activity > 0.01f)
        {
            g.setColour(colour.withAlpha(activity * 0.4f));
            g.fillEllipse(dotPos.x - dotRadius * 2.0f, dotPos.y - dotRadius * 2.0f,
                          dotRadius * 4.0f, dotRadius * 4.0f);
        }

        // Filled dot
        g.setColour(colour);
        g.fillEllipse(dotPos.x - dotRadius, dotPos.y - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);

        // Selected voice gets a highlight ring
        if (i == selectedVoice)
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.drawEllipse(dotPos.x - dotRadius - 2.0f, dotPos.y - dotRadius - 2.0f,
                          (dotRadius + 2.0f) * 2.0f, (dotRadius + 2.0f) * 2.0f, 1.5f);
        }
    }

    // --- Voice Selector Bar ---
    auto barBounds = voiceBarArea.toFloat();

    // Bar background
    g.setColour(OualLookAndFeel::panelColour.darker(0.5f));
    g.fillRect(barBounds);

    float slotWidth = barBounds.getWidth() / static_cast<float>(numVoices);
    float pillMargin = 3.0f;

    for (int i = 0; i < numVoices; ++i)
    {
        auto prefix = getParamPrefix(i);
        bool active = parameters.getRawParameterValue(prefix + "active")->load() > 0.5f;
        auto colour = VoiceElement::getVoiceColour(i);

        auto slotRect = juce::Rectangle<float>(
            barBounds.getX() + static_cast<float>(i) * slotWidth + pillMargin,
            barBounds.getY() + pillMargin,
            slotWidth - pillMargin * 2.0f,
            barBounds.getHeight() - pillMargin * 2.0f);

        if (active)
        {
            // Filled block for active voices
            g.setColour(colour.withAlpha(0.7f));
            g.fillRect(slotRect);
        }
        else
        {
            // Outlined block for inactive voices
            g.setColour(colour.withAlpha(0.3f));
            g.drawRect(slotRect, 1.0f);
        }

        // Selected voice highlight border
        if (i == selectedVoice)
        {
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawRect(slotRect, 2.0f);
        }

    }
}

void LCDDisplay::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    int barHeight = juce::jmax(24, bounds.getHeight() / 7);
    voiceBarArea = bounds.removeFromBottom(barHeight);
    xyPadArea = bounds.reduced(2);
}

void LCDDisplay::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();

    if (voiceBarArea.contains(pos))
    {
        // Clicked in voice bar — determine which voice slot
        float slotWidth = static_cast<float>(voiceBarArea.getWidth()) / static_cast<float>(numVoices);
        int clickedVoice = static_cast<int>(static_cast<float>(pos.x - voiceBarArea.getX()) / slotWidth);
        clickedVoice = juce::jlimit(0, numVoices - 1, clickedVoice);

        if (clickedVoice == selectedVoice)
        {
            // Toggle active state
            auto prefix = getParamPrefix(clickedVoice);
            if (auto* param = parameters.getParameter(prefix + "active"))
            {
                bool active = parameters.getRawParameterValue(prefix + "active")->load() > 0.5f;
                param->setValueNotifyingHost(active ? 0.0f : 1.0f);
            }
        }
        else
        {
            // Select this voice
            setSelectedVoice(clickedVoice);
            if (onVoiceSelected)
                onVoiceSelected(clickedVoice);
        }
    }
    else if (xyPadArea.contains(pos))
    {
        // Check if near an existing voice dot
        int nearVoice = findVoiceNearPoint(pos, 12.0f);
        if (nearVoice >= 0)
        {
            // Start dragging that voice
            selectedVoiceForDrag = nearVoice;
            setSelectedVoice(nearVoice);
            if (onVoiceSelected)
                onVoiceSelected(nearVoice);
        }
        else
        {
            // Move the selected voice's dot to click position (if active)
            auto prefix = getParamPrefix(selectedVoice);
            bool active = parameters.getRawParameterValue(prefix + "active")->load() > 0.5f;
            if (active)
            {
                selectedVoiceForDrag = selectedVoice;
                xyToParam(selectedVoice, pos.toFloat());
            }
        }
    }

    repaint();
}

void LCDDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (selectedVoiceForDrag >= 0)
    {
        auto pos = event.getPosition().toFloat();
        // Clamp to XY pad area
        pos.x = juce::jlimit(static_cast<float>(xyPadArea.getX()),
                              static_cast<float>(xyPadArea.getRight()),
                              pos.x);
        pos.y = juce::jlimit(static_cast<float>(xyPadArea.getY()),
                              static_cast<float>(xyPadArea.getBottom()),
                              pos.y);
        xyToParam(selectedVoiceForDrag, pos);
        repaint();
    }
}

void LCDDisplay::mouseUp(const juce::MouseEvent& /*event*/)
{
    selectedVoiceForDrag = -1;
}

void LCDDisplay::timerCallback()
{
    animationPhase += 0.5f;
    if (animationPhase >= 4.0f)
        animationPhase = 0.0f;

    // Decay activity levels
    for (auto& level : voiceActivityLevels)
    {
        level *= 0.9f;
    }

    repaint();
}

void LCDDisplay::setVoiceActivity(int voiceIndex, float level)
{
    if (voiceIndex >= 0 && voiceIndex < static_cast<int>(voiceActivityLevels.size()))
    {
        voiceActivityLevels[static_cast<size_t>(voiceIndex)] = level;
    }
}

void LCDDisplay::setSelectedVoice(int voice)
{
    selectedVoice = juce::jlimit(0, numVoices - 1, voice);
    repaint();
}
