#include "SamplePlayer.h"

SamplePlayer::SamplePlayer()
{
    std::random_device rd;
    rng.seed(rd());
}

void SamplePlayer::prepare(double sampleRate)
{
    hostSampleRate = sampleRate;
    reset();
}

void SamplePlayer::reset()
{
    playPosition = 0.0;
    playing = false;
}

void SamplePlayer::trigger(float randomAmount, std::optional<float> startPosition)
{
    float range = currentLoopEnd - currentLoopStart;

    if (startPosition.has_value())
    {
        float basePosition = juce::jlimit(currentLoopStart, currentLoopEnd, *startPosition);
        float jitterRange = range * randomAmount * 0.5f;
        float jitter = ((randomDist(rng) * 2.0f) - 1.0f) * jitterRange;
        playPosition = juce::jlimit(currentLoopStart, currentLoopEnd, basePosition + jitter);
    }
    else
    {
        float randomOffset = randomDist(rng) * randomAmount;
        playPosition = currentLoopStart + randomOffset * range;
    }

    playing = true;
}

float SamplePlayer::process(const juce::AudioBuffer<float>& sampleBuffer,
                            double sampleSourceRate,
                            float speed,
                            float loopStart,
                            float loopEnd)
{
    if (!playing || sampleBuffer.getNumSamples() == 0)
        return 0.0f;

    currentLoopStart = loopStart;
    currentLoopEnd = loopEnd;

    int numSamples = sampleBuffer.getNumSamples();

    // Convert normalized positions to sample indices
    double startSample = loopStart * numSamples;
    double endSample = loopEnd * numSamples;

    // Clamp end sample
    endSample = juce::jmin(endSample, static_cast<double>(numSamples - 1));

    // Ensure valid range
    if (startSample >= endSample)
        return 0.0f;

    // Convert playPosition from normalized to sample index
    double currentSamplePos = playPosition * numSamples;

    // Linear interpolation for smooth playback
    int idx0 = static_cast<int>(currentSamplePos);
    int idx1 = idx0 + 1;
    float frac = static_cast<float>(currentSamplePos - idx0);

    // Clamp indices
    idx0 = juce::jlimit(0, numSamples - 1, idx0);
    idx1 = juce::jlimit(0, numSamples - 1, idx1);

    // Read sample (mono mix if stereo)
    float sample0, sample1;
    if (sampleBuffer.getNumChannels() >= 2)
    {
        sample0 = (sampleBuffer.getSample(0, idx0) + sampleBuffer.getSample(1, idx0)) * 0.5f;
        sample1 = (sampleBuffer.getSample(0, idx1) + sampleBuffer.getSample(1, idx1)) * 0.5f;
    }
    else
    {
        sample0 = sampleBuffer.getSample(0, idx0);
        sample1 = sampleBuffer.getSample(0, idx1);
    }

    float output = sample0 + frac * (sample1 - sample0);

    // Advance position
    // Speed 0.5 = normal, 0 = slowest, 1 = fastest (reversed mapping from original)
    double speedMultiplier = 0.25 + speed * 1.75; // Range: 0.25x to 2x
    double sampleRateRatio = sampleSourceRate / hostSampleRate;
    double increment = (speedMultiplier * sampleRateRatio) / numSamples;

    playPosition += increment;

    // Loop check
    if (playPosition >= loopEnd || playPosition < loopStart)
    {
        // Loop back to start
        playPosition = loopStart;
    }

    return output;
}
