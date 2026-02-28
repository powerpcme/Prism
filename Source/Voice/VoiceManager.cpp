#include "VoiceManager.h"

VoiceManager::VoiceManager(juce::AudioProcessorValueTreeState& params, int numVoices)
    : parameters(params)
{
    voices.reserve(static_cast<size_t>(numVoices));
    for (int i = 0; i < numVoices; ++i)
    {
        voices.push_back(std::make_unique<Voice>(i, params));
    }
}

void VoiceManager::prepare(double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    blockSize = samplesPerBlock;

    for (auto& voice : voices)
    {
        voice->prepare(sampleRate, samplesPerBlock);
    }
}

void VoiceManager::reset()
{
    for (auto& voice : voices)
    {
        voice->reset();
    }
}

void VoiceManager::updateAllParameters()
{
    for (auto& voice : voices)
    {
        voice->updateParameters();
    }
}

void VoiceManager::process(juce::AudioBuffer<float>& buffer,
                           const std::array<juce::AudioBuffer<float>, 10>& sampleBuffers,
                           const std::array<double, 10>& sampleSourceRates,
                           const std::array<std::shared_ptr<const std::vector<float>>, 10>& markers,
                           TempoSync& tempoSync)
{
    int numSamples = buffer.getNumSamples();

    // Update parameters once per block
    updateAllParameters();

    // Process all voices - each voice handles its own triggering
    for (size_t i = 0; i < voices.size(); ++i)
    {
        if (sampleBuffers[i].getNumSamples() > 0)
        {
            voices[i]->process(buffer, sampleBuffers[i], sampleSourceRates[i],
                               markers[i],
                               tempoSync, 0, numSamples);
        }
    }
}
