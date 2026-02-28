#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <optional>
#include <random>

class SamplePlayer
{
public:
    SamplePlayer();

    void prepare(double sampleRate);
    void reset();

    // Trigger playback from loop start with optional random offset
    void trigger(float randomAmount = 0.0f, std::optional<float> startPosition = std::nullopt);

    // Process a single sample with given speed
    float process(const juce::AudioBuffer<float>& sampleBuffer,
                  double sampleSourceRate,
                  float speed,
                  float loopStart,
                  float loopEnd);

    bool isPlaying() const { return playing; }
    double getPlayPosition() const { return playPosition; }

private:
    double hostSampleRate = 44100.0;
    double playPosition = 0.0;
    bool playing = false;

    float currentLoopStart = 0.0f;
    float currentLoopEnd = 1.0f;

    std::mt19937 rng;
    std::uniform_real_distribution<float> randomDist{0.0f, 1.0f};
};
