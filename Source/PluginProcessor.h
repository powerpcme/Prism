#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Voice/VoiceManager.h"
#include "Sync/TempoSync.h"
#include <array>

class PrismProcessor : public juce::AudioProcessor
{
public:
    PrismProcessor();
    ~PrismProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Per-voice sample loading
    void loadSampleForVoice(int voiceIndex, const juce::File& file);
    bool hasSampleLoaded(int voiceIndex) const;
    bool hasAnySampleLoaded() const;
    const juce::AudioBuffer<float>& getSampleBuffer(int voiceIndex) const { return sampleBuffers[static_cast<size_t>(voiceIndex)]; }
    double getSampleSampleRate(int voiceIndex) const { return sampleSourceRates[static_cast<size_t>(voiceIndex)]; }
    const juce::String& getSampleName(int voiceIndex) const { return sampleNames[static_cast<size_t>(voiceIndex)]; }
    const juce::String& getSampleFilePath(int voiceIndex) const { return sampleFilePaths[static_cast<size_t>(voiceIndex)]; }

    // Playback position access (delegates to voice)
    double getVoicePlayPosition(int voiceIndex) const;

    // Modulated loop positions (for waveform display)
    float getVoiceModLoopStart(int voiceIndex) const;

    // Sync access
    TempoSync& getTempoSync() { return tempoSync; }

    // Voice selection for UI
    int getSelectedVoice() const { return selectedVoice; }
    void setSelectedVoice(int voice) { selectedVoice = juce::jlimit(0, 9, voice); }

    // MIDI learn
    void setMidiLearnMode(bool enabled) { midiLearnMode = enabled; }
    bool isMidiLearnMode() const { return midiLearnMode; }
    void setMidiLearnTarget(const juce::String& paramId);

    // Preset management
    void savePreset(const juce::File& file);
    void loadPreset(const juce::File& file);
    juce::File getPresetsFolder() const;

    // Undo/Redo
    juce::UndoManager& getUndoManager() { return undoManager; }

    static constexpr int NUM_VOICES = 10;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void handleMidiMessage(const juce::MidiMessage& message);

    juce::AudioProcessorValueTreeState parameters;
    juce::UndoManager undoManager;

    // Per-voice sample data
    std::array<juce::AudioBuffer<float>, 10> sampleBuffers;
    std::array<double, 10> sampleSourceRates;
    std::array<juce::String, 10> sampleNames;
    std::array<juce::String, 10> sampleFilePaths;
    juce::AudioFormatManager formatManager;

    // Voice management
    std::unique_ptr<VoiceManager> voiceManager;

    // Tempo sync
    TempoSync tempoSync;

    // UI state
    int selectedVoice = 0;

    // MIDI learn
    bool midiLearnMode = false;
    juce::String midiLearnTargetParam;
    std::map<int, juce::String> midiCCMappings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismProcessor)
};
