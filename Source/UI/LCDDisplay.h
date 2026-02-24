#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "VoiceElement.h"
#include <array>

class LCDDisplay : public juce::Component, public juce::Timer
{
public:
    LCDDisplay(juce::AudioProcessorValueTreeState& params, int numVoices);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void timerCallback() override;

    // Callback when a voice is selected
    std::function<void(int)> onVoiceSelected;

    // Update voice activity levels
    void setVoiceActivity(int voiceIndex, float level);

    // Get selected voice
    int getSelectedVoice() const { return selectedVoice; }
    void setSelectedVoice(int voice);

private:
    juce::AudioProcessorValueTreeState& parameters;
    int numVoices;

    int selectedVoice = 0;
    int selectedVoiceForDrag = -1;

    // Cached layout areas
    juce::Rectangle<int> xyPadArea;
    juce::Rectangle<int> voiceBarArea;

    // Activity levels for visualization
    std::array<float, 10> voiceActivityLevels{};

    // Background animation
    float animationPhase = 0.0f;

    // Helper to get param prefix
    juce::String getParamPrefix(int voiceIndex) const;

    // Convert between XY pad coordinates and parameter values
    juce::Point<float> paramToXY(int voiceIndex) const;
    void xyToParam(int voiceIndex, juce::Point<float> xyPos);

    // Find which voice dot is near a point (returns -1 if none)
    int findVoiceNearPoint(juce::Point<int> point, float radius) const;
};
