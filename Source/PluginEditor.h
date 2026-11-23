#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class EnvelopeDisplay final : public juce::Component
{
public:
    void setSliders (juce::Slider& attack, juce::Slider& decay,
                     juce::Slider& sustain, juce::Slider& release);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    enum class Handle { none, attack, decay, sustain, release };

    juce::Slider* attackSlider = nullptr;
    juce::Slider* decaySlider = nullptr;
    juce::Slider* sustainSlider = nullptr;
    juce::Slider* releaseSlider = nullptr;
    Handle draggingHandle = Handle::none;

    std::array<juce::Point<float>, 5> getEnvelopePoints() const;
    juce::Rectangle<float> getPlotBounds() const;
    void setSliderValue (juce::Slider* slider, double value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplay)
};

class WaveformDisplay final : public juce::Component
{
public:
    void setWaveform (int newWaveform, float newMorph);
    void paint (juce::Graphics&) override;

private:
    int waveform = 0;
    float morph = 0.0f;

    static float waveformValue (int type, float phase) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};


class SampleWaveformDisplay final : public juce::Component
{
public:
    explicit SampleWaveformDisplay (HybridSynthAudioProcessor& processor);
    void setRangeSliders (juce::Slider& start, juce::Slider& end);
    void paint (juce::Graphics&) override;

private:
    HybridSynthAudioProcessor& audioProcessor;
    juce::Slider* startSlider = nullptr;
    juce::Slider* endSlider = nullptr;

    void drawWaveform (juce::Graphics& g, const juce::AudioBuffer<float>& audio,
                       float start, float end, juce::Colour colour,
                       juce::Rectangle<float> plot) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleWaveformDisplay)
};

class HybridSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HybridSynthAudioProcessorEditor (HybridSynthAudioProcessor&);
    ~HybridSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HybridSynthAudioProcessor& audioProcessor;
    bool showingUser = false;

    juce::Label titleLabel;
    juce::TextButton synthTab { "SYNTH" };
    juce::TextButton userTab { "USER" };
    juce::TextButton settingsButton { "⚙" };

    juce::Label voiceModeLabel;
    juce::ComboBox voiceModeBox;
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;

    juce::Label sourceTitle;
    juce::ComboBox waveformBox;
    WaveformDisplay waveformDisplay;
    juce::Slider morphSlider;
    juce::Slider wavetableLevelSlider;
    juce::Label morphLabel;
    juce::Label wavetableLevelLabel;

    juce::Label sampleTitle;
    juce::TextButton importSampleButton { "IMPORT SAMPLE" };
    juce::Label sampleNameLabel;
    SampleWaveformDisplay sampleWaveformDisplay;
    juce::Slider sampleLevelSlider;
    juce::Slider sampleStartSlider;
    juce::Slider sampleEndSlider;
    juce::Slider sampleRootSlider;
    juce::Label sampleLevelLabel;
    juce::Label sampleStartLabel;
    juce::Label sampleEndLabel;
    juce::Label sampleRootLabel;
    juce::ToggleButton sampleLoopButton { "LOOP" };

    juce::Label envelopeTitle;
    EnvelopeDisplay envelopeDisplay;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    juce::TextButton randomizeEnvelopeButton { "RANDOMIZE ENVELOPE" };
    juce::TextButton randomizeWavetableButton { "RANDOMIZE WAVETABLE" };
    juce::TextButton randomizeSampleButton { "RANDOMIZE SAMPLE" };
    juce::TextButton randomizeAllButton { "RANDOMIZE ALL" };

    juce::Label userTitle;
    juce::Label userDescription;
    juce::ComboBox librarySamplesBox;
    juce::TextButton refreshLibraryButton { "REFRESH" };
    juce::TextButton loadLibrarySampleButton { "LOAD SELECTED" };
    juce::TextButton importLibrarySampleButton { "IMPORT FILE" };
    juce::TextButton deleteLibrarySampleButton { "DELETE SELECTED" };
    juce::TextButton clearLoadedSampleButton { "CLEAR LOADED SAMPLE" };
    juce::Label libraryStatusLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboAttachment> voiceModeAttachment;
    std::unique_ptr<ComboAttachment> waveformAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
        sustainAttachment, releaseAttachment, masterGainAttachment;
    std::unique_ptr<SliderAttachment> morphAttachment, wavetableLevelAttachment;
    std::unique_ptr<SliderAttachment> sampleLevelAttachment, sampleStartAttachment,
        sampleEndAttachment, sampleRootAttachment;
    std::unique_ptr<ButtonAttachment> sampleLoopAttachment;

    std::unique_ptr<juce::FileChooser> sampleChooser;

    void configureSlider (juce::Slider& slider, juce::Label& label,
                          const juce::String& labelText);
    void configureLabel (juce::Label& label, const juce::String& text);
    void showTab (bool user);
    void openSampleChooser();
    void refreshLibraryList();
    juce::File getSelectedLibraryFile() const;
    void updateSampleLabels();
    void showTechnicalSettingsMessage();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridSynthAudioProcessorEditor)
};
