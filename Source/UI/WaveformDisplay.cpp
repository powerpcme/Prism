#include "WaveformDisplay.h"
#include "../PluginProcessor.h"
#include "LookAndFeel.h"

WaveformDisplay::WaveformDisplay(PrismProcessor& p, juce::AudioProcessorValueTreeState& params)
    : processor(p), parameters(params)
{
    startTimerHz(30);
}

void WaveformDisplay::setVoice(int v)
{
    voiceIndex = v;
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
        int startSample = static_cast<int>((static_cast<double>(col) / w) * numSamples);
        int endSample = static_cast<int>((static_cast<double>(col + 1) / w) * numSamples);

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
    if (startX > 0)
        g.fillRect(0.0f, 0.0f, startX, static_cast<float>(h));
    if (endX < w)
        g.fillRect(endX, 0.0f, w - endX, static_cast<float>(h));

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
    g.setColour(juce::Colours::green);
    g.drawVerticalLine(static_cast<int>(startX), 0.0f, static_cast<float>(h));
    // Triangle handle at top
    juce::Path startTriangle;
    startTriangle.addTriangle(startX, 0.0f, startX + 6.0f, 0.0f, startX, 10.0f);
    g.fillPath(startTriangle);

    // End marker (red)
    g.setColour(juce::Colours::red);
    g.drawVerticalLine(static_cast<int>(endX), 0.0f, static_cast<float>(h));
    // Triangle handle at top
    juce::Path endTriangle;
    endTriangle.addTriangle(endX, 0.0f, endX - 6.0f, 0.0f, endX, 10.0f);
    g.fillPath(endTriangle);

    // Modulated start marker (translucent)
    float modStart = processor.getVoiceModLoopStart(voiceIndex);

    if (std::abs(modStart - loopStart) > 0.001f)
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

    // Playhead (white line)
    double playPos = processor.getVoicePlayPosition(voiceIndex);
    if (playPos > 0.0 && processor.hasSampleLoaded(voiceIndex))
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
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingStart && !draggingEnd)
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

    repaint();
}

void WaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    draggingStart = false;
    draggingEnd = false;
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
    return x / static_cast<float>(getWidth());
}

float WaveformDisplay::normalizedToX(float normalized) const
{
    return normalized * static_cast<float>(getWidth());
}
