#include "TempoSync.h"
#include <cmath>

void TempoSync::prepare(double sampleRate)
{
    hostSampleRate = sampleRate;
    samplesUntilNextTrigger = 0;
}

void TempoSync::updateFromPlayhead(const juce::AudioPlayHead::PositionInfo& posInfo)
{
    if (auto bpmOpt = posInfo.getBpm())
        bpm = *bpmOpt;

    if (auto ppqOpt = posInfo.getPpqPosition())
    {
        lastPpqPosition = ppqPosition;
        ppqPosition = *ppqOpt;
    }

    playing = posInfo.getIsPlaying();

    // Recalculate sync interval based on sync rate
    syncInterval = SyncDivisions::getMultiplier(syncRate);
}

void TempoSync::setSyncRate(SyncRate rate)
{
    syncRate = rate;
    syncInterval = SyncDivisions::getMultiplier(rate);
}

int TempoSync::getSyncIntervalSamples() const
{
    if (bpm <= 0.0)
        return static_cast<int>(hostSampleRate); // Default to 1 second

    double beatsPerSecond = bpm / 60.0;
    double samplesPerBeat = hostSampleRate / beatsPerSecond;
    return static_cast<int>(samplesPerBeat * syncInterval);
}

double TempoSync::getSyncIntervalMs() const
{
    if (bpm <= 0.0)
        return 1000.0;

    double beatsPerSecond = bpm / 60.0;
    double msPerBeat = 1000.0 / beatsPerSecond;
    return msPerBeat * syncInterval;
}

bool TempoSync::shouldTrigger(int sampleOffset)
{
    // Handle manual trigger
    if (manualTrigger)
    {
        manualTrigger = false;
        return true;
    }

    if (!playing || bpm <= 0.0)
        return false;

    // Calculate PPQ position at this sample offset
    double beatsPerSecond = bpm / 60.0;
    double beatsPerSample = beatsPerSecond / hostSampleRate;
    double currentPpq = ppqPosition + (sampleOffset * beatsPerSample);

    // Check if we crossed a sync boundary
    double currentBeat = std::fmod(currentPpq, syncInterval);
    double previousBeat = std::fmod(ppqPosition + ((sampleOffset - 1) * beatsPerSample), syncInterval);

    // Trigger when we cross from end of interval to start
    // This handles wrap-around properly
    if (sampleOffset == 0)
    {
        previousBeat = std::fmod(lastPpqPosition, syncInterval);
    }

    // Check for boundary crossing
    // We trigger if current position wrapped around (currentBeat < previousBeat)
    // or if this is the exact start of an interval
    bool crossedBoundary = currentBeat < previousBeat;
    bool atStart = currentBeat < beatsPerSample;

    return crossedBoundary || (sampleOffset == 0 && atStart && playing);
}
