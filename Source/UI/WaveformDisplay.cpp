#include "WaveformDisplay.h"
#include "../PluginProcessor.h"
#include "LookAndFeel.h"
#include <algorithm>

WaveformDisplay::WaveformDisplay(PrismProcessor& p, juce::AudioProcessorValueTreeState& params)
    : processor(p), parameters(params)
{
    startTimerHz(30);
}

void WaveformDisplay::setVoice(int v)
{
    voiceIndex = v;
    resetZoom();
    cachedNumSamples = 0; // force rebuild
    rebuildWaveformCache();
    repaint();
}

void WaveformDisplay::rebuildWaveformCache()
{
    auto& buffer = processor.getSampleBuffer(voiceIndex);
    int numSamples = buffer.getNumSamples();
    int w = getWidth();

    waveformPeaks.clear();

    if (numSamples == 0 || w <= 0)
        return;

    waveformPeaks.resize(static_cast<size_t>(w));
    cachedNumSamples = numSamples;

    int numChannels = buffer.getNumChannels();

    for (int col = 0; col < w; ++col)
    {
        auto colStartNorm = viewStart + (static_cast<float>(col) / static_cast<float>(w)) * (viewEnd - viewStart);
        auto colEndNorm = viewStart + (static_cast<float>(col + 1) / static_cast<float>(w)) * (viewEnd - viewStart);
        int startSample = static_cast<int>(colStartNorm * numSamples);
        int endSample = static_cast<int>(colEndNorm * numSamples);

        if (endSample > numSamples)
            endSample = numSamples;
        if (startSample >= endSample)
            startSample = endSample - 1;
        if (startSample < 0)
            startSample = 0;

        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getReadPointer(ch);
            for (int s = startSample; s < endSample; ++s)
            {
                float val = data[s];
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
        }

        waveformPeaks[static_cast<size_t>(col)] = { minVal, maxVal };
    }
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background
    g.setColour(OualLookAndFeel::backgroundColour.darker(0.3f));
    g.fillRect(bounds);

    // Border
    g.setColour(OualLookAndFeel::accentColour.withAlpha(0.3f));
    g.drawRect(bounds.reduced(0.5f), 1.0f);

    int w = getWidth();
    int h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    float midY = h * 0.5f;

    // Get loop start/end parameter values
    juce::String prefix = "voice_" + juce::String(voiceIndex) + "_";
    float loopStart = parameters.getRawParameterValue(prefix + "loop_start")->load();
    float loopEnd = parameters.getRawParameterValue(prefix + "loop_end")->load();

    float startX = normalizedToX(loopStart);
    float endX = normalizedToX(loopEnd);

    // Draw shaded regions outside loop
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    if (loopStart > viewStart)
        g.fillRect(0.0f, 0.0f, juce::jlimit(0.0f, static_cast<float>(w), startX), static_cast<float>(h));
    if (loopEnd < viewEnd)
        g.fillRect(juce::jlimit(0.0f, static_cast<float>(w), endX), 0.0f,
                   static_cast<float>(w) - juce::jlimit(0.0f, static_cast<float>(w), endX), static_cast<float>(h));

    // Draw waveform
    if (!waveformPeaks.empty())
    {
        g.setColour(OualLookAndFeel::accentColour.withAlpha(0.8f));

        for (int col = 0; col < w && col < static_cast<int>(waveformPeaks.size()); ++col)
        {
            auto [minVal, maxVal] = waveformPeaks[static_cast<size_t>(col)];
            float y1 = midY - maxVal * midY;
            float y2 = midY - minVal * midY;
            g.drawVerticalLine(col, y1, y2);
        }
    }
    else
    {
        // No sample loaded - draw hint text
        g.setColour(OualLookAndFeel::textColour.withAlpha(0.3f));
        g.setFont(12.0f);
        g.drawText("No sample loaded", bounds, juce::Justification::centred);
    }

    // Start marker (green)
    if (loopStart >= viewStart && loopStart <= viewEnd)
    {
        g.setColour(juce::Colours::green);
        g.drawVerticalLine(static_cast<int>(startX), 0.0f, static_cast<float>(h));
        juce::Path startTriangle;
        startTriangle.addTriangle(startX, 0.0f, startX + 6.0f, 0.0f, startX, 10.0f);
        g.fillPath(startTriangle);
    }

    // End marker (red)
    if (loopEnd >= viewStart && loopEnd <= viewEnd)
    {
        g.setColour(juce::Colours::red);
        g.drawVerticalLine(static_cast<int>(endX), 0.0f, static_cast<float>(h));
        juce::Path endTriangle;
        endTriangle.addTriangle(endX, 0.0f, endX - 6.0f, 0.0f, endX, 10.0f);
        g.fillPath(endTriangle);
    }

    // Modulated start marker (translucent)
    float modStart = processor.getVoiceModLoopStart(voiceIndex);

    if (std::abs(modStart - loopStart) > 0.001f && modStart >= viewStart && modStart <= viewEnd)
    {
        float modStartX = normalizedToX(modStart);
        g.setColour(juce::Colours::green.withAlpha(0.4f));
        g.drawVerticalLine(static_cast<int>(modStartX), 0.0f, static_cast<float>(h));
        // Small triangle handle
        juce::Path modStartTri;
        modStartTri.addTriangle(modStartX, static_cast<float>(h),
                                modStartX + 4.0f, static_cast<float>(h),
                                modStartX, static_cast<float>(h) - 7.0f);
        g.fillPath(modStartTri);
    }

    // User start markers
    auto markers = processor.getVoiceMarkers(voiceIndex);
    g.setColour(juce::Colour(0xffffb347).withAlpha(0.85f));
    for (float marker : markers)
    {
        if (marker < viewStart || marker > viewEnd)
            continue;

        float markerX = normalizedToX(marker);
        g.drawVerticalLine(static_cast<int>(markerX), 0.0f, static_cast<float>(h));

        juce::Path markerTriangle;
        markerTriangle.addTriangle(markerX - 4.0f, static_cast<float>(h),
                                   markerX + 4.0f, static_cast<float>(h),
                                   markerX, static_cast<float>(h) - 8.0f);
        g.fillPath(markerTriangle);
    }

    // Playhead (white line)
    double playPos = processor.getVoicePlayPosition(voiceIndex);
    if (playPos > 0.0 && processor.hasSampleLoaded(voiceIndex) && playPos >= viewStart && playPos <= viewEnd)
    {
        float playX = normalizedToX(static_cast<float>(playPos));
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawVerticalLine(static_cast<int>(playX), 0.0f, static_cast<float>(h));
    }
}

void WaveformDisplay::resized()
{
    rebuildWaveformCache();
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isMiddleButtonDown() && processor.hasSampleLoaded(voiceIndex) && (viewEnd - viewStart) < 0.999f)
    {
        panningView = true;
        panAnchorX = static_cast<float>(e.getPosition().getX());
        panAnchorStart = viewStart;
        panAnchorEnd = viewEnd;
        return;
    }

    juce::String prefix = "voice_" + juce::String(voiceIndex) + "_";
    float loopStart = parameters.getRawParameterValue(prefix + "loop_start")->load();
    float loopEnd = parameters.getRawParameterValue(prefix + "loop_end")->load();

    float startX = normalizedToX(loopStart);
    float endX = normalizedToX(loopEnd);
    float mouseX = static_cast<float>(e.getPosition().getX());

    float distToStart = std::abs(mouseX - startX);
    float distToEnd = std::abs(mouseX - endX);

    if (distToStart < markerHitZone && distToStart <= distToEnd)
        draggingStart = true;
    else if (distToEnd < markerHitZone)
        draggingEnd = true;
    else
        draggingMarkerIndex = findMarkerNearX(mouseX);
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (panningView)
    {
        auto width = static_cast<float>(juce::jmax(1, getWidth()));
        auto span = panAnchorEnd - panAnchorStart;
        auto deltaX = static_cast<float>(e.getPosition().getX()) - panAnchorX;
        auto deltaNorm = (deltaX / width) * span;

        viewStart = panAnchorStart - deltaNorm;
        viewEnd = panAnchorEnd - deltaNorm;

        if (viewStart < 0.0f)
        {
            viewEnd -= viewStart;
            viewStart = 0.0f;
        }
        if (viewEnd > 1.0f)
        {
            viewStart -= (viewEnd - 1.0f);
            viewEnd = 1.0f;
        }

        viewStart = juce::jlimit(0.0f, 1.0f - span, viewStart);
        viewEnd = juce::jlimit(span, 1.0f, viewStart + span);

        rebuildWaveformCache();
        repaint();
        return;
    }

    if (!draggingStart && !draggingEnd && draggingMarkerIndex < 0)
        return;

    juce::String prefix = "voice_" + juce::String(voiceIndex) + "_";
    float normalized = xToNormalized(static_cast<float>(e.getPosition().getX()));
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    if (draggingStart)
    {
        float loopEnd = parameters.getRawParameterValue(prefix + "loop_end")->load();
        normalized = juce::jmin(normalized, loopEnd - 0.001f);
        if (auto* param = parameters.getParameter(prefix + "loop_start"))
            param->setValueNotifyingHost(param->convertTo0to1(normalized));
    }
    else if (draggingEnd)
    {
        float loopStart = parameters.getRawParameterValue(prefix + "loop_start")->load();
        normalized = juce::jmax(normalized, loopStart + 0.001f);
        if (auto* param = parameters.getParameter(prefix + "loop_end"))
            param->setValueNotifyingHost(param->convertTo0to1(normalized));
    }
    else if (draggingMarkerIndex >= 0)
    {
        auto markers = processor.getVoiceMarkers(voiceIndex);
        if (draggingMarkerIndex < static_cast<int>(markers.size()))
        {
            markers[static_cast<size_t>(draggingMarkerIndex)] = normalized;
            processor.replaceVoiceMarkers(voiceIndex, std::move(markers));
        }
    }

    repaint();
}

void WaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    draggingStart = false;
    draggingEnd = false;
    draggingMarkerIndex = -1;
    panningView = false;
}

void WaveformDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!processor.hasSampleLoaded(voiceIndex))
        return;

    processor.toggleVoiceMarker(voiceIndex, xToNormalized(static_cast<float>(e.getPosition().getX())));
    repaint();
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (!e.mods.isCtrlDown())
    {
        juce::Component::mouseWheelMove(e, wheel);
        return;
    }

    if (!processor.hasSampleLoaded(voiceIndex))
        return;

    auto mouseNorm = xToNormalized(static_cast<float>(e.getPosition().getX()));
    auto currentSpan = viewEnd - viewStart;
    auto zoomDelta = 1.0f - (wheel.deltaY * 0.25f);
    auto newSpan = juce::jlimit(0.02f, 1.0f, currentSpan * zoomDelta);

    if (newSpan >= 0.999f)
    {
        resetZoom();
    }
    else
    {
        auto focusRatio = (mouseNorm - viewStart) / currentSpan;
        focusRatio = juce::jlimit(0.0f, 1.0f, focusRatio);

        viewStart = mouseNorm - focusRatio * newSpan;
        viewEnd = viewStart + newSpan;

        if (viewStart < 0.0f)
        {
            viewEnd -= viewStart;
            viewStart = 0.0f;
        }
        if (viewEnd > 1.0f)
        {
            viewStart -= (viewEnd - 1.0f);
            viewEnd = 1.0f;
        }

        viewStart = juce::jlimit(0.0f, 1.0f - newSpan, viewStart);
        viewEnd = juce::jlimit(newSpan, 1.0f, viewStart + newSpan);
    }

    rebuildWaveformCache();
    repaint();
}

void WaveformDisplay::timerCallback()
{
    // Check if sample changed (rebuild cache)
    auto& buffer = processor.getSampleBuffer(voiceIndex);
    if (buffer.getNumSamples() != cachedNumSamples)
        rebuildWaveformCache();

    repaint();
}

float WaveformDisplay::xToNormalized(float x) const
{
    auto width = static_cast<float>(juce::jmax(1, getWidth()));
    auto local = juce::jlimit(0.0f, width, x) / width;
    return viewStart + local * (viewEnd - viewStart);
}

float WaveformDisplay::normalizedToX(float normalized) const
{
    auto span = juce::jmax(0.0001f, viewEnd - viewStart);
    return ((normalized - viewStart) / span) * static_cast<float>(getWidth());
}

void WaveformDisplay::resetZoom()
{
    viewStart = 0.0f;
    viewEnd = 1.0f;
}

int WaveformDisplay::findMarkerNearX(float x) const
{
    auto markers = processor.getVoiceMarkers(voiceIndex);
    for (size_t i = 0; i < markers.size(); ++i)
    {
        if (markers[i] < viewStart || markers[i] > viewEnd)
            continue;

        if (std::abs(normalizedToX(markers[i]) - x) <= markerHitZone)
            return static_cast<int>(i);
    }

    return -1;
}
