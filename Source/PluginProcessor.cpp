#include "PluginProcessor.h"
#include "PluginEditor.h"

PrismProcessor::PrismProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, &undoManager, juce::Identifier("Prism"), createParameterLayout())
{
    formatManager.registerBasicFormats();
    voiceManager = std::make_unique<VoiceManager>(parameters, NUM_VOICES);
    sampleSourceRates.fill(44100.0);
}

PrismProcessor::~PrismProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PrismProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto floatAttrs = juce::AudioParameterFloatAttributes().withAutomatable(true);
    const auto boolAttrs = juce::AudioParameterBoolAttributes().withAutomatable(true);
    const auto choiceAttrs = juce::AudioParameterChoiceAttributes().withAutomatable(true);

    auto addFloat = [&] (juce::AudioProcessorParameterGroup& group,
                         const juce::String& id,
                         const juce::String& name,
                         juce::NormalisableRange<float> range,
                         float defaultValue)
    {
        group.addChild(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name, range, defaultValue, floatAttrs));
    };

    auto addBool = [&] (juce::AudioProcessorParameterGroup& group,
                        const juce::String& id,
                        const juce::String& name,
                        bool defaultValue)
    {
        group.addChild(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(id, 1), name, defaultValue, boolAttrs));
    };

    auto addChoice = [&] (juce::AudioProcessorParameterGroup& group,
                          const juce::String& id,
                          const juce::String& name,
                          juce::StringArray choices,
                          int defaultIndex)
    {
        group.addChild(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(id, 1), name, std::move(choices), defaultIndex, choiceAttrs));
    };

    auto globalGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
        "global", "Global", " / ");

    // Global parameters
    addFloat(*globalGroup, "master_gain", "Master Gain",
             juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f);
    layout.add(std::move(globalGroup));

    // Per-voice parameters (10 voices)
    for (int v = 0; v < NUM_VOICES; ++v)
    {
        juce::String prefix = "voice_" + juce::String(v) + "_";
        auto voiceGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
            "voice_" + juce::String(v), "Voice " + juce::String(v), " / ");

        addFloat(*voiceGroup, prefix + "speed", "Voice " + juce::String(v) + " Speed",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f);
        addFloat(*voiceGroup, prefix + "drive", "Voice " + juce::String(v) + " Drive",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
        addFloat(*voiceGroup, prefix + "srr", "Voice " + juce::String(v) + " SRR",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
        addFloat(*voiceGroup, prefix + "bitrate", "Voice " + juce::String(v) + " Bitrate",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f);
        addFloat(*voiceGroup, prefix + "rand_pos", "Voice " + juce::String(v) + " Random Position",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
        addFloat(*voiceGroup, prefix + "loop_start", "Voice " + juce::String(v) + " Loop Start",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f);
        addFloat(*voiceGroup, prefix + "loop_end", "Voice " + juce::String(v) + " Loop End",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f);
        addFloat(*voiceGroup, prefix + "am", "Voice " + juce::String(v) + " AM",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
        addFloat(*voiceGroup, prefix + "envelope", "Voice " + juce::String(v) + " Envelope",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f);
        addFloat(*voiceGroup, prefix + "env_rate", "Voice " + juce::String(v) + " Env Rate",
                 juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f);
        addBool(*voiceGroup, prefix + "active", "Voice " + juce::String(v) + " Active", false);
        addBool(*voiceGroup, prefix + "sync_enabled", "Voice " + juce::String(v) + " Sync Enabled", true);
        addChoice(*voiceGroup, prefix + "sync_rate", "Voice " + juce::String(v) + " Sync Rate",
                  {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8T", "1/16T"}, 4);
        addFloat(*voiceGroup, prefix + "rand_start", "Voice " + juce::String(v) + " Rand Start",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f);
        addChoice(*voiceGroup, prefix + "rand_rate", "Voice " + juce::String(v) + " Rand Rate",
                  {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8T", "1/16T"}, 2);
        addFloat(*voiceGroup, prefix + "pan", "Voice " + juce::String(v) + " Pan",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f);
        addFloat(*voiceGroup, prefix + "volume", "Voice " + juce::String(v) + " Volume",
                 juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f);

        layout.add(std::move(voiceGroup));
    }

    return layout;
}

const juce::String PrismProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PrismProcessor::acceptsMidi() const
{
    return true;
}

bool PrismProcessor::producesMidi() const
{
    return false;
}

bool PrismProcessor::isMidiEffect() const
{
    return false;
}

double PrismProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PrismProcessor::getNumPrograms()
{
    return 1;
}

int PrismProcessor::getCurrentProgram()
{
    return 0;
}

void PrismProcessor::setCurrentProgram(int /*index*/)
{
}

const juce::String PrismProcessor::getProgramName(int /*index*/)
{
    return {};
}

void PrismProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/)
{
}

void PrismProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    tempoSync.prepare(sampleRate);
    voiceManager->prepare(sampleRate, samplesPerBlock);
}

void PrismProcessor::releaseResources()
{
}

bool PrismProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void PrismProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Handle MIDI messages
    for (const auto metadata : midiMessages)
    {
        handleMidiMessage(metadata.getMessage());
    }

    // Update tempo sync from playhead
    if (auto* ph = getPlayHead())
    {
        if (auto posInfo = ph->getPosition())
        {
            tempoSync.updateFromPlayhead(*posInfo);
        }
    }

    // Process voices (each voice handles its own sync)
    if (hasAnySampleLoaded())
    {
        voiceManager->process(buffer, sampleBuffers, sampleSourceRates, tempoSync);
    }

    // Apply master gain
    float masterGain = parameters.getRawParameterValue("master_gain")->load();
    buffer.applyGain(masterGain);
}

void PrismProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    if (message.isController())
    {
        int cc = message.getControllerNumber();
        int value = message.getControllerValue();

        // MIDI Learn mode
        if (midiLearnMode && !midiLearnTargetParam.isEmpty())
        {
            midiCCMappings[cc] = midiLearnTargetParam;
            midiLearnMode = false;
            midiLearnTargetParam = "";
            return;
        }

        // Apply existing mappings
        auto it = midiCCMappings.find(cc);
        if (it != midiCCMappings.end())
        {
            if (auto* param = parameters.getParameter(it->second))
            {
                param->setValueNotifyingHost(value / 127.0f);
            }
        }
    }
}

void PrismProcessor::setMidiLearnTarget(const juce::String& paramId)
{
    midiLearnTargetParam = paramId;
    midiLearnMode = true;
}

bool PrismProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PrismProcessor::createEditor()
{
    return new PrismEditor(*this);
}

void PrismProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Add MIDI mappings to state
    juce::ValueTree midiState("MidiMappings");
    for (const auto& [cc, paramId] : midiCCMappings)
    {
        juce::ValueTree mapping("Mapping");
        mapping.setProperty("cc", cc, nullptr);
        mapping.setProperty("param", paramId, nullptr);
        midiState.addChild(mapping, -1, nullptr);
    }
    state.addChild(midiState, -1, nullptr);

    // Add sample file paths to state so they can be reloaded on project open
    juce::ValueTree samplesState("SamplePaths");
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        auto idx = static_cast<size_t>(i);
        if (sampleFilePaths[idx].isNotEmpty())
        {
            juce::ValueTree sampleEntry("Voice");
            sampleEntry.setProperty("index", i, nullptr);
            sampleEntry.setProperty("path", sampleFilePaths[idx], nullptr);
            samplesState.addChild(sampleEntry, -1, nullptr);
        }
    }
    state.addChild(samplesState, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PrismProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);

            // Extract MIDI mappings
            auto midiState = newState.getChildWithName("MidiMappings");
            if (midiState.isValid())
            {
                midiCCMappings.clear();
                for (int i = 0; i < midiState.getNumChildren(); ++i)
                {
                    auto mapping = midiState.getChild(i);
                    int cc = mapping.getProperty("cc");
                    juce::String paramId = mapping.getProperty("param");
                    midiCCMappings[cc] = paramId;
                }
                newState.removeChild(midiState, nullptr);
            }

            // Extract and reload sample file paths
            auto samplesState = newState.getChildWithName("SamplePaths");
            if (samplesState.isValid())
            {
                for (int i = 0; i < samplesState.getNumChildren(); ++i)
                {
                    auto sampleEntry = samplesState.getChild(i);
                    int voiceIndex = sampleEntry.getProperty("index");
                    juce::String filePath = sampleEntry.getProperty("path");

                    juce::File sampleFile(filePath);
                    if (sampleFile.existsAsFile())
                    {
                        loadSampleForVoice(voiceIndex, sampleFile);
                    }
                }
                newState.removeChild(samplesState, nullptr);
            }

            parameters.replaceState(newState);
        }
    }
}

void PrismProcessor::loadSampleForVoice(int voiceIndex, const juce::File& file)
{
    if (voiceIndex < 0 || voiceIndex >= NUM_VOICES)
        return;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        auto idx = static_cast<size_t>(voiceIndex);
        sampleSourceRates[idx] = reader->sampleRate;
        sampleBuffers[idx].setSize(static_cast<int>(reader->numChannels),
                                    static_cast<int>(reader->lengthInSamples));
        reader->read(&sampleBuffers[idx], 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        sampleNames[idx] = file.getFileNameWithoutExtension();
        sampleFilePaths[idx] = file.getFullPathName();
    }
}

bool PrismProcessor::hasSampleLoaded(int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= NUM_VOICES)
        return false;
    return sampleBuffers[static_cast<size_t>(voiceIndex)].getNumSamples() > 0;
}

bool PrismProcessor::hasAnySampleLoaded() const
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        if (sampleBuffers[static_cast<size_t>(i)].getNumSamples() > 0)
            return true;
    }
    return false;
}

double PrismProcessor::getVoicePlayPosition(int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= NUM_VOICES)
        return 0.0;
    return voiceManager->getVoice(voiceIndex).getPlayPosition();
}

float PrismProcessor::getVoiceModLoopStart(int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= NUM_VOICES)
        return 0.0f;
    return voiceManager->getVoice(voiceIndex).getModLoopStart();
}

void PrismProcessor::savePreset(const juce::File& file)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    if (xml != nullptr)
    {
        xml->writeTo(file);
    }
}

void PrismProcessor::loadPreset(const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse(file))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::File PrismProcessor::getPresetsFolder() const
{
    auto documentsFolder = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory);
    auto presetsFolder = documentsFolder.getChildFile("Prism").getChildFile("Presets");

    if (!presetsFolder.exists())
        presetsFolder.createDirectory();

    return presetsFolder;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrismProcessor();
}
