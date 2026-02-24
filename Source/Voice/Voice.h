#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SamplePlayer.h"
#include "../DSP/RateShifter.h"
#include "../DSP/Overdrive.h"
#include "../DSP/BitCrusher.h"
#include "../DSP/AMModulator.h"
#include "../DSP/EnvelopeShaper.h"
#include "../Sync/TempoSync.h"
#include <random>

class Voice
{
public:
    Voice(int voiceIndex, juce::AudioProcessorValueTreeState& params);

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Trigger voice playback
    void trigger();

    // Process a block of samples (self-triggering with per-voice sync)
    void process(juce::AudioBuffer<float>& buffer,
                 const juce::AudioBuffer<float>& sampleBuffer,
                 double sampleSourceRate,
                 TempoSync& tempoSync,
                 int startSample,
                 int numSamples);

    // Update parameters from APVTS
    void updateParameters();

    // Check if voice is active (playing)
    bool isActive() const { return active && envelope.isActive(); }

    // Voice index
    int getIndex() const { return voiceIndex; }

    // Playback position (0..1 normalized)
    double getPlayPosition() const { return samplePlayer.getPlayPosition(); }

    // Modulated loop positions (for waveform display)
    float getModLoopStart() const { return modLoopStart; }

private:
    int voiceIndex;
    juce::AudioProcessorValueTreeState& parameters;
    double hostSampleRate = 44100.0;

    // DSP chain
    SamplePlayer samplePlayer;
    RateShifter rateShifter;
    Overdrive overdrive;
    BitCrusher bitCrusher;
    AMModulator amModulator;
    EnvelopeShaper envelope;

    // Parameter values
    float speed = 0.5f;
    float drive = 0.0f;
    float srr = 0.0f;
    float bitrate = 1.0f;
    float randPos = 0.0f;
    float loopStart = 0.0f;
    float loopEnd = 1.0f;
    float am = 0.0f;
    float envelopeAmount = 1.0f;
    float envRate = 0.0f;
    bool active = false;

    // Per-voice sync
    bool syncEnabled = true;
    int syncRateIndex = 4; // default 1/16
    int samplesSinceTrigger = 0;
    int freeRunningInterval = 22050;

    // Pan and volume
    float pan = 0.5f;
    float volume = 0.8f;

    // Random start/end modulation
    float randStart = 0.0f;
    int randRateIndex = 2; // default 1/4

    // Modulated loop start (updated at rand_rate ticks)
    float modLoopStart = 0.0f;

    // Random engine for start/end modulation
    std::mt19937 randModRng { std::random_device{}() };
    std::uniform_real_distribution<float> randModDist { 0.0f, 1.0f };

    // Parameter ID helpers
    juce::String getParamId(const juce::String& suffix) const;
};
