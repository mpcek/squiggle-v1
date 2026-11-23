#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include <atomic>
#include <memory>
#include <vector>

class HybridSynthAudioProcessor final : public juce::AudioProcessor
{
public:
    HybridSynthAudioProcessor();
    ~HybridSynthAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Sample Library operations are called from the message thread.
    juce::File getUserSamplesDirectory() const;
    juce::Array<juce::File> getUserSampleFiles() const;
    bool importSampleToLibrary (const juce::File& sourceFile);
    bool loadSampleFromFile (const juce::File& file);
    bool deleteUserSample (const juce::File& file);
    void clearSample();
    juce::String getLoadedSampleName() const;
    juce::String getLoadedSamplePath() const;
    std::shared_ptr<const SampleData> getLoadedSampleData() const;

    void randomizeEnvelope();
    void randomizeWavetable();
    void randomizeSample();
    void randomizeAll();

private:
    static constexpr int maximumVoices = 16;

    juce::Synthesiser polySynth;
    juce::Synthesiser monoSynth;
    WavetableBank wavetableBank;
    juce::AudioFormatManager formatManager;

    std::vector<HybridSynthVoice*> polyVoices;
    std::vector<HybridSynthVoice*> monoVoices;

    // Atomic shared_ptr publication lets the UI replace a sample without taking
    // a mutex in the audio callback. Retired buffers stay alive until destruction
    // so their memory is not freed by the audio thread after a voice swaps buffers.
    std::shared_ptr<const SampleData> loadedSample;
    std::vector<std::shared_ptr<const SampleData>> retiredSamples;

    std::atomic<float>* voiceModeParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* masterGainParam = nullptr;
    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* morphParam = nullptr;
    std::atomic<float>* wavetableLevelParam = nullptr;
    std::atomic<float>* sampleLevelParam = nullptr;
    std::atomic<float>* sampleStartParam = nullptr;
    std::atomic<float>* sampleEndParam = nullptr;
    std::atomic<float>* sampleRootNoteParam = nullptr;
    std::atomic<float>* sampleLoopParam = nullptr;

    int lastVoiceMode = 0;
    float lastGain = 1.0f;

    void publishSample (std::shared_ptr<const SampleData> newSample);
    void setParameterPlainValue (const juce::String& parameterID, float plainValue);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridSynthAudioProcessor)
};
