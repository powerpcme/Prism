#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Voice.h"
#include "../Sync/TempoSync.h"
#include <array>

class VoiceManager
{
public:
    VoiceManager(juce::AudioProcessorValueTreeState& params, int numVoices);

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process a block of audio with per-voice sample buffers
    void process(juce::AudioBuffer<float>& buffer,
                 const std::array<juce::AudioBuffer<float>, 10>& sampleBuffers,
                 const std::array<double, 10>& sampleSourceRates,
                 TempoSync& tempoSync);

    // Get a voice by index
    Voice& getVoice(int index) { return *voices[static_cast<size_t>(index)]; }
    const Voice& getVoice(int index) const { return *voices[static_cast<size_t>(index)]; }

    int getNumVoices() const { return static_cast<int>(voices.size()); }

private:
    void updateAllParameters();

    juce::AudioProcessorValueTreeState& parameters;
    std::vector<std::unique_ptr<Voice>> voices;

    double hostSampleRate = 44100.0;
    int blockSize = 512;
};
