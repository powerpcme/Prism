#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SyncDivisions.h"

class TempoSync
{
public:
    TempoSync() = default;

    void prepare(double sampleRate);

    // Update from DAW playhead info
    void updateFromPlayhead(const juce::AudioPlayHead::PositionInfo& posInfo);

    // Set the sync rate
    void setSyncRate(SyncRate rate);

    // Get the interval in samples for the current sync rate
    int getSyncIntervalSamples() const;

    // Get the interval in milliseconds
    double getSyncIntervalMs() const;

    // Check if we should trigger at this sample position
    // Returns true once per sync interval, aligned to DAW grid
    bool shouldTrigger(int sampleOffset);

    // Getters for current state
    double getBpm() const { return bpm; }
    bool isPlaying() const { return playing; }
    double getPpqPosition() const { return ppqPosition; }
    SyncRate getSyncRate() const { return syncRate; }

    // Manual trigger for when sync is disabled
    void triggerNow() { manualTrigger = true; }

private:
    double hostSampleRate = 44100.0;
    double bpm = 120.0;
    double ppqPosition = 0.0;
    bool playing = false;
    SyncRate syncRate = SyncRate::RATE_1_16;

    // For trigger detection
    double lastPpqPosition = 0.0;
    double syncInterval = 0.25; // In beats (quarter notes)
    bool manualTrigger = false;

    // For sample-accurate triggering
    int samplesUntilNextTrigger = 0;
    int currentSampleInBlock = 0;
};
