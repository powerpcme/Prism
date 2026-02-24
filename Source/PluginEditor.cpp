#include "PluginEditor.h"

PrismEditor::PrismEditor(PrismProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&lookAndFeel);

    // Set up resizable constraints (40:33 aspect ratio to fit waveform strip)
    constrainer.setFixedAspectRatio(800.0 / 660.0);
    constrainer.setMinimumSize(600, 495);
    constrainer.setMaximumSize(1600, 1320);

    // Header - Title
    titleLabel.setText("Prism", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f).boldened());
    titleLabel.setColour(juce::Label::textColourId, OualLookAndFeel::accentColour);
    addAndMakeVisible(titleLabel);

    // Load sample button (loads for selected voice)
    loadSampleButton.setButtonText("Load Sample");
    loadSampleButton.onClick = [this]() {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a sample file",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff;*.mp3;*.flac");

        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                processorRef.loadSampleForVoice(selectedVoice, file);
            }
        });
    };
    addAndMakeVisible(loadSampleButton);

    // LCD Display
    lcdDisplay = std::make_unique<LCDDisplay>(processorRef.getParameters(), PrismProcessor::NUM_VOICES);
    lcdDisplay->onVoiceSelected = [this](int voice) {
        selectedVoice = voice;
        processorRef.setSelectedVoice(voice);
        updateSelectedVoiceControls();
    };
    addAndMakeVisible(*lcdDisplay);

    // Selected voice label
    selectedVoiceLabel.setText("Voice: Red", juce::dontSendNotification);
    selectedVoiceLabel.setFont(juce::Font(16.0f).boldened());
    addAndMakeVisible(selectedVoiceLabel);

    // Create sliders for selected voice parameters
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.attachToComponent(&slider, true);
        addAndMakeVisible(label);
    };

    setupSlider(speedSlider, speedLabel, "Speed");
    setupSlider(driveSlider, driveLabel, "Drive");
    setupSlider(crushSlider, crushLabel, "Crush");
    setupSlider(amSlider, amLabel, "AM");
    setupSlider(envelopeSlider, envelopeLabel, "Envelope");
    setupSlider(envRateSlider, envRateLabel, "Env Rate");
    setupSlider(randPosSlider, randPosLabel, "Rnd Pos");
    setupSlider(randStartSlider, randStartLabel, "Rnd Start");

    setupSlider(panSlider, panLabel, "Pan");
    setupSlider(volumeSlider, volumeLabel, "Volume");

    // Rand rate combo
    randRateLabel.setText("Rnd Rate", juce::dontSendNotification);
    addAndMakeVisible(randRateLabel);
    randRateCombo.addItemList({"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8T", "1/16T"}, 1);
    addAndMakeVisible(randRateCombo);

    // Waveform display (replaces start/end sliders)
    waveformDisplay = std::make_unique<WaveformDisplay>(processorRef, processorRef.getParameters());
    addAndMakeVisible(*waveformDisplay);

    // Sample name label
    sampleNameLabel.setFont(juce::Font(11.0f));
    sampleNameLabel.setColour(juce::Label::textColourId, OualLookAndFeel::textColour.withAlpha(0.7f));
    sampleNameLabel.setMinimumHorizontalScale(0.5f);
    addAndMakeVisible(sampleNameLabel);

    // Active toggle
    activeButton.setButtonText("Active");
    addAndMakeVisible(activeButton);

    // Per-voice sync controls
    syncEnableButton.setButtonText("SYNC");
    addAndMakeVisible(syncEnableButton);

    syncRateCombo.addItemList({"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8T", "1/16T"}, 1);
    addAndMakeVisible(syncRateCombo);

    // Resizer
    resizer = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(*resizer);

    // Initial voice selection
    updateSelectedVoiceControls();

    // Start UI update timer
    startTimerHz(30);

    // Set initial size
    setSize(800, 660);
    setResizable(true, true);
}

PrismEditor::~PrismEditor()
{
    setLookAndFeel(nullptr);
}

void PrismEditor::paint(juce::Graphics& g)
{
    float scale = static_cast<float>(getWidth()) / 800.0f;
    int controlsHeight = static_cast<int>(240 * scale);
    int waveformHeight = static_cast<int>(60 * scale);

    g.fillAll(OualLookAndFeel::backgroundColour);

    // Draw header background
    auto headerBounds = getLocalBounds().removeFromTop(50);
    g.setColour(OualLookAndFeel::panelColour);
    g.fillRect(headerBounds);

    // Draw controls panel background
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // skip header
    auto bottomArea = bounds.removeFromBottom(controlsHeight + waveformHeight);
    auto controlsPanel = bottomArea.removeFromTop(controlsHeight);
    g.setColour(OualLookAndFeel::panelColour.darker(0.2f));
    g.fillRect(controlsPanel);

    // Draw waveform strip background
    auto waveformStrip = bottomArea;
    g.setColour(OualLookAndFeel::panelColour.darker(0.4f));
    g.fillRect(waveformStrip);

    // Separator lines
    g.setColour(OualLookAndFeel::accentColour.withAlpha(0.3f));
    g.drawHorizontalLine(50, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - controlsHeight - waveformHeight, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - waveformHeight, 0.0f, static_cast<float>(getWidth()));
}

void PrismEditor::resized()
{
    auto bounds = getLocalBounds();
    float scale = static_cast<float>(getWidth()) / 800.0f;

    // Header
    auto header = bounds.removeFromTop(static_cast<int>(50 * scale));
    titleLabel.setBounds(header.removeFromLeft(static_cast<int>(120 * scale)).reduced(static_cast<int>(10 * scale)));

    auto headerRight = header.removeFromRight(static_cast<int>(140 * scale));
    loadSampleButton.setBounds(headerRight.reduced(static_cast<int>(5 * scale)));

    // Waveform strip at the very bottom
    int waveformHeight = static_cast<int>(60 * scale);
    auto waveformStrip = bounds.removeFromBottom(waveformHeight);
    waveformDisplay->setBounds(waveformStrip.reduced(static_cast<int>(10 * scale), static_cast<int>(4 * scale)));

    // Controls panel above waveform
    auto bottomPanel = bounds.removeFromBottom(static_cast<int>(240 * scale));
    auto controlsArea = bottomPanel.reduced(static_cast<int>(20 * scale));

    // Voice label, active button, sync controls, rand rate, sample name on top row
    auto topRow = controlsArea.removeFromTop(static_cast<int>(30 * scale));
    selectedVoiceLabel.setBounds(topRow.removeFromLeft(static_cast<int>(100 * scale)));
    activeButton.setBounds(topRow.removeFromLeft(static_cast<int>(80 * scale)));
    syncEnableButton.setBounds(topRow.removeFromLeft(static_cast<int>(60 * scale)));
    syncRateCombo.setBounds(topRow.removeFromLeft(static_cast<int>(80 * scale)));

    // Rand rate combo in the top row
    auto randRateLabelArea = topRow.removeFromLeft(static_cast<int>(60 * scale));
    randRateLabel.setBounds(randRateLabelArea);
    randRateCombo.setBounds(topRow.removeFromLeft(static_cast<int>(80 * scale)));

    sampleNameLabel.setBounds(topRow.reduced(static_cast<int>(4 * scale), 0));

    // Sliders in 3 columns, 4 rows
    int sliderHeight = static_cast<int>(35 * scale);
    int labelWidth = static_cast<int>(70 * scale);
    int columnWidth = (controlsArea.getWidth() - labelWidth * 3) / 3;

    // Row 1: Speed | Drive | Crush
    auto row1 = controlsArea.removeFromTop(sliderHeight);
    speedSlider.setBounds(row1.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    driveSlider.setBounds(row1.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    crushSlider.setBounds(row1.withTrimmedLeft(labelWidth));

    // Row 2: Envelope | Env Rate | AM
    auto row2 = controlsArea.removeFromTop(sliderHeight);
    envelopeSlider.setBounds(row2.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    envRateSlider.setBounds(row2.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    amSlider.setBounds(row2.withTrimmedLeft(labelWidth));

    // Row 3: Rand Pos | Rand Start
    auto row3 = controlsArea.removeFromTop(sliderHeight);
    randPosSlider.setBounds(row3.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    randStartSlider.setBounds(row3.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));

    // Row 4: Pan | Volume
    auto row4 = controlsArea.removeFromTop(sliderHeight);
    panSlider.setBounds(row4.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));
    volumeSlider.setBounds(row4.removeFromLeft(columnWidth + labelWidth).withTrimmedLeft(labelWidth));

    // LCD Display (main area)
    lcdDisplay->setBounds(bounds.reduced(static_cast<int>(10 * scale)));

    // Resizer
    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
}

void PrismEditor::timerCallback()
{
    // Update sample name in case it was loaded since last check
    sampleNameLabel.setText(processorRef.getSampleName(selectedVoice), juce::dontSendNotification);
    repaint();
}

void PrismEditor::updateSelectedVoiceControls()
{
    juce::String prefix = "voice_" + juce::String(selectedVoice) + "_";

    // Update label with color name
    selectedVoiceLabel.setText("Voice: " + VoiceElement::getKeyLabel(selectedVoice),
                                juce::dontSendNotification);

    // Clear old attachments
    speedAttachment.reset();
    driveAttachment.reset();
    crushAttachment.reset();
    amAttachment.reset();
    envelopeAttachment.reset();
    envRateAttachment.reset();
    randPosAttachment.reset();
    randStartAttachment.reset();
    panAttachment.reset();
    volumeAttachment.reset();
    activeAttachment.reset();
    syncEnableAttachment.reset();
    syncRateAttachment.reset();
    randRateAttachment.reset();

    // Create new attachments for selected voice
    auto& params = processorRef.getParameters();

    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "speed", speedSlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "drive", driveSlider);
    crushAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "srr", crushSlider);
    amAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "am", amSlider);
    envelopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "envelope", envelopeSlider);
    envRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "env_rate", envRateSlider);
    randPosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "rand_pos", randPosSlider);
    randStartAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "rand_start", randStartSlider);
    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "pan", panSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, prefix + "volume", volumeSlider);
    activeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, prefix + "active", activeButton);

    // Per-voice sync attachments
    syncEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, prefix + "sync_enabled", syncEnableButton);
    syncRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, prefix + "sync_rate", syncRateCombo);
    randRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, prefix + "rand_rate", randRateCombo);

    // Update waveform display and sample name
    waveformDisplay->setVoice(selectedVoice);
    sampleNameLabel.setText(processorRef.getSampleName(selectedVoice), juce::dontSendNotification);
}

bool PrismEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (file.endsWithIgnoreCase(".wav") ||
            file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") ||
            file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".flac"))
        {
            return true;
        }
    }
    return false;
}

void PrismEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    for (const auto& file : files)
    {
        juce::File audioFile(file);
        if (audioFile.existsAsFile())
        {
            processorRef.loadSampleForVoice(selectedVoice, audioFile);
            break; // Only load first valid file
        }
    }
}
