#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/LCDDisplay.h"
#include "UI/WaveformDisplay.h"

class PrismEditor : public juce::AudioProcessorEditor,
                    public juce::Timer,
                    public juce::FileDragAndDropTarget
{
public:
    explicit PrismEditor(PrismProcessor&);
    ~PrismEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Timer for UI updates
    void timerCallback() override;

    // File drag and drop for sample loading
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    void updateSelectedVoiceControls();
    void createParameterSlider(const juce::String& paramId, const juce::String& label,
                               juce::Slider& slider, juce::Label& labelComponent,
                               std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment);

    PrismProcessor& processorRef;
    OualLookAndFeel lookAndFeel;

    // FileChooser kept alive for async callbacks
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Header components
    juce::Label titleLabel;
    juce::TextButton loadSampleButton;

    // Main display
    std::unique_ptr<LCDDisplay> lcdDisplay;

    // Selected voice parameter controls
    juce::Label selectedVoiceLabel;

    juce::Slider speedSlider, driveSlider, crushSlider;
    juce::Slider amSlider;
    juce::Slider envelopeSlider, envRateSlider, randPosSlider;
    juce::Slider randStartSlider;
    juce::Slider panSlider, volumeSlider;

    // Waveform display replaces start/end sliders
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    juce::Label sampleNameLabel;
    juce::ToggleButton activeButton;

    // Per-voice sync controls
    juce::ToggleButton syncEnableButton;
    juce::ComboBox syncRateCombo;

    // Rand rate combo
    juce::ComboBox randRateCombo;
    juce::Label randRateLabel;

    juce::Label speedLabel, driveLabel, crushLabel;
    juce::Label amLabel;
    juce::Label envelopeLabel, envRateLabel, randPosLabel;
    juce::Label randStartLabel;
    juce::Label panLabel, volumeLabel;

    // Per-voice parameter attachments (updated when selection changes)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envelopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> randPosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> randStartAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> activeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> randRateAttachment;

    // Resizing
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;

    int selectedVoice = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismEditor)
};
