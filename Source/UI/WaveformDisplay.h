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
    void mouseUp(const juce::MouseEvent& e) override;
    void timerCallback() override;

private:
    void rebuildWaveformCache();
    float xToNormalized(float x) const;
    float normalizedToX(float normalized) const;

    PrismProcessor& processor;
    juce::AudioProcessorValueTreeState& parameters;
    int voiceIndex = 0;

    // Cached waveform peaks: min/max per pixel column
    std::vector<std::pair<float, float>> waveformPeaks;
    int cachedNumSamples = 0;

    // Drag state
    bool draggingStart = false;
    bool draggingEnd = false;

    static constexpr int markerHitZone = 6;
};
