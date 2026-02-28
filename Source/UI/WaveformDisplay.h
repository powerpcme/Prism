#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class PrismProcessor;

class WaveformDisplay : public juce::Component, public juce::Timer
{
public:
    WaveformDisplay(PrismProcessor& processor, juce::AudioProcessorValueTreeState& parameters);

    void setVoice(int voiceIndex);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void timerCallback() override;

private:
    void rebuildWaveformCache();
    void resetZoom();
    float xToNormalized(float x) const;
    float normalizedToX(float normalized) const;
    int findMarkerNearX(float x) const;

    PrismProcessor& processor;
    juce::AudioProcessorValueTreeState& parameters;
    int voiceIndex = 0;

    // Cached waveform peaks: min/max per pixel column
    std::vector<std::pair<float, float>> waveformPeaks;
    int cachedNumSamples = 0;

    // Drag state
    bool draggingStart = false;
    bool draggingEnd = false;
    int draggingMarkerIndex = -1;
    bool panningView = false;
    float panAnchorX = 0.0f;
    float panAnchorStart = 0.0f;
    float panAnchorEnd = 1.0f;
    float viewStart = 0.0f;
    float viewEnd = 1.0f;

    static constexpr int markerHitZone = 6;
};
