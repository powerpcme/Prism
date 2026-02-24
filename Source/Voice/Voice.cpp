#include "Voice.h"

Voice::Voice(int index, juce::AudioProcessorValueTreeState& params)
    : voiceIndex(index), parameters(params)
{
}

juce::String Voice::getParamId(const juce::String& suffix) const
{
    return "voice_" + juce::String(voiceIndex) + "_" + suffix;
}

void Voice::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    hostSampleRate = sampleRate;
    freeRunningInterval = static_cast<int>(sampleRate / 2.0); // 500ms default

    samplePlayer.prepare(sampleRate);
    rateShifter.prepare(sampleRate);
    overdrive.prepare(sampleRate);
    bitCrusher.prepare(sampleRate);
    amModulator.prepare(sampleRate);
    envelope.prepare(sampleRate);
}

void Voice::reset()
{
    samplePlayer.reset();
    rateShifter.reset();
    overdrive.reset();
    bitCrusher.reset();
    amModulator.reset();
    envelope.reset();
    samplesSinceTrigger = 0;
}

void Voice::updateParameters()
{
    speed = parameters.getRawParameterValue(getParamId("speed"))->load();
    drive = parameters.getRawParameterValue(getParamId("drive"))->load();
    srr = parameters.getRawParameterValue(getParamId("srr"))->load();
    bitrate = parameters.getRawParameterValue(getParamId("bitrate"))->load();
    randPos = parameters.getRawParameterValue(getParamId("rand_pos"))->load();
    loopStart = parameters.getRawParameterValue(getParamId("loop_start"))->load();
    loopEnd = parameters.getRawParameterValue(getParamId("loop_end"))->load();
    am = parameters.getRawParameterValue(getParamId("am"))->load();
    envelopeAmount = parameters.getRawParameterValue(getParamId("envelope"))->load();
    envRate = parameters.getRawParameterValue(getParamId("env_rate"))->load();
    active = parameters.getRawParameterValue(getParamId("active"))->load() > 0.5f;

    // Pan and volume
    pan = parameters.getRawParameterValue(getParamId("pan"))->load();
    volume = parameters.getRawParameterValue(getParamId("volume"))->load();

    // Per-voice sync params
    syncEnabled = parameters.getRawParameterValue(getParamId("sync_enabled"))->load() > 0.5f;
    syncRateIndex = static_cast<int>(parameters.getRawParameterValue(getParamId("sync_rate"))->load());

    // Random start modulation params
    randStart = parameters.getRawParameterValue(getParamId("rand_start"))->load();
    randRateIndex = static_cast<int>(parameters.getRawParameterValue(getParamId("rand_rate"))->load());

    // Update DSP components
    rateShifter.setPitchShift(speed);
    overdrive.setDrive(drive);
    bitCrusher.setSampleRateReduction(srr);
    bitCrusher.setBitDepth(bitrate);
    amModulator.setDepth(am);
    envelope.setEnvelopeAmount(envelopeAmount);
    envelope.setEnvelopeRate(envRate);
}

void Voice::trigger()
{
    if (!active)
        return;

    samplePlayer.trigger(randPos);
    envelope.trigger();
}

void Voice::process(juce::AudioBuffer<float>& buffer,
                    const juce::AudioBuffer<float>& sampleBuffer,
                    double sampleSourceRate,
                    TempoSync& tempoSync,
                    int startSample,
                    int numSamples)
{
    if (!active)
        return;

    // Self-triggering: each voice handles its own sync
    for (int i = 0; i < numSamples; ++i)
    {
        bool shouldTriggerNow = false;

        if (syncEnabled)
        {
            // Use the shared TempoSync but with this voice's own rate
            // Temporarily set the sync rate, check trigger, then we process
            auto savedRate = tempoSync.getSyncRate();
            tempoSync.setSyncRate(static_cast<SyncRate>(syncRateIndex));
            shouldTriggerNow = tempoSync.shouldTrigger(i);

            // Check rand_rate boundary for random start modulation
            if (randStart > 0.0f)
            {
                tempoSync.setSyncRate(static_cast<SyncRate>(randRateIndex));
                bool randTick = tempoSync.shouldTrigger(i);
                if (randTick)
                {
                    float range = loopEnd - loopStart;
                    modLoopStart = loopStart + randModDist(randModRng) * randStart * range;
                    // Clamp so modLoopStart stays before loopEnd
                    modLoopStart = juce::jlimit(0.0f, loopEnd - 0.01f, modLoopStart);
                }
            }
            else
            {
                modLoopStart = loopStart;
            }

            tempoSync.setSyncRate(savedRate);
        }
        else
        {
            // Free-running mode per voice
            samplesSinceTrigger++;
            if (samplesSinceTrigger >= freeRunningInterval)
            {
                shouldTriggerNow = true;
                samplesSinceTrigger = 0;
            }

            // No beat sync in free-running, just pass through
            if (randStart <= 0.0f)
            {
                modLoopStart = loopStart;
            }
        }

        if (shouldTriggerNow)
        {
            trigger();
        }
    }

    // Process audio if envelope is active
    if (!envelope.isActive())
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        int sampleIndex = startSample + i;

        // Get sample from player (use modulated start, raw end)
        float sample = samplePlayer.process(sampleBuffer, sampleSourceRate,
                                             speed, modLoopStart, loopEnd);

        // Overdrive
        sample = overdrive.process(sample);

        // Bit crushing
        sample = bitCrusher.process(sample);

        // AM modulation
        sample = amModulator.process(sample);

        // Apply envelope
        float envValue = envelope.process();
        sample *= envValue;

        // Apply volume
        sample *= volume;

        // Apply stereo panning (constant-power)
        float leftGain  = std::cos(pan * juce::MathConstants<float>::halfPi);
        float rightGain = std::sin(pan * juce::MathConstants<float>::halfPi);

        if (buffer.getNumChannels() >= 2)
        {
            buffer.addSample(0, sampleIndex, sample * leftGain);
            buffer.addSample(1, sampleIndex, sample * rightGain);
        }
        else
        {
            buffer.addSample(0, sampleIndex, sample);
        }
    }
}
