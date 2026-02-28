#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
int getHeaderHeight(int height)
{
    return juce::jlimit(48, 72, juce::roundToInt(static_cast<float>(height) * 0.08f));
}

int getWaveformHeight(int height)
{
    return juce::jlimit(60, 120, juce::roundToInt(static_cast<float>(height) * 0.10f));
}

int getControlsHeight(int height)
{
    return juce::jlimit(210, 320, juce::roundToInt(static_cast<float>(height) * 0.36f));
}

int getPadding(int width, int height)
{
    return juce::jlimit(8, 20, juce::roundToInt(static_cast<float>(juce::jmin(width, height)) * 0.015f));
}
}

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

    auto logoImage = juce::ImageCache::getFromMemory(BinaryData::PrismLogo_png,
                                                     BinaryData::PrismLogo_pngSize);
    if (logoImage.isValid())
    {
        logoComponent.setImage(logoImage);
        logoComponent.setImagePlacement(juce::RectanglePlacement::xLeft
                                        | juce::RectanglePlacement::yMid
                                        | juce::RectanglePlacement::onlyReduceInSize);
        addAndMakeVisible(logoComponent);
        hasLogo = true;
        titleLabel.setVisible(false);
    }

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

    markerModeButton.setButtonText("Markers");
    addAndMakeVisible(markerModeButton);

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
    g.fillAll(OualLookAndFeel::backgroundColour);

    auto bounds = getLocalBounds();
    auto headerHeight = getHeaderHeight(bounds.getHeight());
    auto waveformHeight = getWaveformHeight(bounds.getHeight());
    auto controlsHeight = getControlsHeight(bounds.getHeight());
    auto headerBounds = bounds.removeFromTop(headerHeight);

    g.setColour(OualLookAndFeel::panelColour);
    g.fillRect(headerBounds);

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
    g.drawHorizontalLine(headerHeight, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - controlsHeight - waveformHeight, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - waveformHeight, 0.0f, static_cast<float>(getWidth()));
}

void PrismEditor::resized()
{
    auto bounds = getLocalBounds();
    auto headerHeight = getHeaderHeight(bounds.getHeight());
    auto waveformHeight = getWaveformHeight(bounds.getHeight());
    auto controlsHeight = getControlsHeight(bounds.getHeight());
    auto padding = getPadding(bounds.getWidth(), bounds.getHeight());
    auto compactPadding = juce::jmax(6, padding - 2);

    // Header
    auto header = bounds.removeFromTop(headerHeight);
    auto logoWidth = juce::jlimit(180, juce::jmax(180, getWidth() / 3), getWidth() / 3);
    auto buttonWidth = juce::jlimit(120, 170, getWidth() / 5);

    auto logoArea = header.removeFromLeft(logoWidth);
    if (hasLogo)
        logoComponent.setBounds(logoArea.reduced(padding, compactPadding));
    else
        titleLabel.setBounds(logoArea.reduced(padding));

    auto headerRight = header.removeFromRight(buttonWidth);
    loadSampleButton.setBounds(headerRight.reduced(compactPadding));

    // Waveform strip at the very bottom
    auto waveformStrip = bounds.removeFromBottom(waveformHeight);
    waveformDisplay->setBounds(waveformStrip.reduced(padding, compactPadding));

    // Controls panel above waveform
    auto bottomPanel = bounds.removeFromBottom(controlsHeight);
    auto controlsArea = bottomPanel.reduced(padding * 2, padding);

    // Voice label, active button, sync controls, rand rate, sample name on top row
    auto topRowHeight = juce::jlimit(28, 36, controlsArea.getHeight() / 5);
    auto topRow = controlsArea.removeFromTop(topRowHeight);
    auto topGap = juce::jmax(6, padding / 2);

    selectedVoiceLabel.setBounds(topRow.removeFromLeft(juce::jlimit(100, 150, controlsArea.getWidth() / 6)));
    topRow.removeFromLeft(topGap);
    activeButton.setBounds(topRow.removeFromLeft(juce::jlimit(72, 96, controlsArea.getWidth() / 10)));
    topRow.removeFromLeft(topGap);
    markerModeButton.setBounds(topRow.removeFromLeft(juce::jlimit(78, 110, controlsArea.getWidth() / 9)));
    topRow.removeFromLeft(topGap);
    syncEnableButton.setBounds(topRow.removeFromLeft(juce::jlimit(56, 80, controlsArea.getWidth() / 12)));
    topRow.removeFromLeft(topGap);
    syncRateCombo.setBounds(topRow.removeFromLeft(juce::jlimit(76, 110, controlsArea.getWidth() / 10)));
    topRow.removeFromLeft(topGap);

    // Rand rate combo in the top row
    auto randRateLabelArea = topRow.removeFromLeft(juce::jlimit(56, 76, controlsArea.getWidth() / 11));
    randRateLabel.setBounds(randRateLabelArea);
    randRateCombo.setBounds(topRow.removeFromLeft(juce::jlimit(76, 110, controlsArea.getWidth() / 10)));

    sampleNameLabel.setBounds(topRow.reduced(topGap, 0));

    controlsArea.removeFromTop(juce::jmax(8, padding / 2));

    // Sliders in 3 columns, 4 rows
    auto rowGap = juce::jmax(8, padding / 2);
    auto totalRowGap = rowGap * 3;
    auto sliderHeight = juce::jlimit(30, 42, (controlsArea.getHeight() - totalRowGap) / 4);
    auto labelWidth = juce::jlimit(56, 78, controlsArea.getWidth() / 9);
    auto columnGap = juce::jmax(10, padding);
    auto columnWidth = juce::jmax(120, (controlsArea.getWidth() - (columnGap * 2)) / 3);

    auto layoutSliderRow = [labelWidth, columnGap, columnWidth]
        (juce::Rectangle<int> row, std::initializer_list<juce::Slider*> sliders)
    {
        auto iterator = sliders.begin();
        for (int column = 0; column < 3 && iterator != sliders.end(); ++column, ++iterator)
        {
            auto cell = row.removeFromLeft(columnWidth);
            if (*iterator != nullptr)
                (*iterator)->setBounds(cell.withTrimmedLeft(labelWidth));

            if (column < 2)
                row.removeFromLeft(columnGap);
        }
    };

    // Row 1: Speed | Drive | Crush
    layoutSliderRow(controlsArea.removeFromTop(sliderHeight), { &speedSlider, &driveSlider, &crushSlider });
    controlsArea.removeFromTop(rowGap);

    // Row 2: Envelope | Env Rate | AM
    layoutSliderRow(controlsArea.removeFromTop(sliderHeight), { &envelopeSlider, &envRateSlider, &amSlider });
    controlsArea.removeFromTop(rowGap);

    // Row 3: Rand Pos | Rand Start
    layoutSliderRow(controlsArea.removeFromTop(sliderHeight), { &randPosSlider, &randStartSlider });
    controlsArea.removeFromTop(rowGap);

    // Row 4: Pan | Volume
    layoutSliderRow(controlsArea.removeFromTop(sliderHeight), { &panSlider, &volumeSlider });

    // LCD Display (main area)
    lcdDisplay->setBounds(bounds.reduced(padding));

    // Resizer
    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
}

void PrismEditor::timerCallback()
{
    // Update sample name in case it was loaded since last check
    auto sampleName = processorRef.getSampleName(selectedVoice);
    if (sampleNameLabel.getText() != sampleName)
        sampleNameLabel.setText(sampleName, juce::dontSendNotification);
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
    markerModeAttachment.reset();
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
    markerModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        params, prefix + "marker_mode", markerModeButton);

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
