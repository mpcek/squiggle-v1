#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

class HybridSynthSound final : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

struct SampleData
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::File file;
    juce::String name;
};

// A small band-limited wavetable bank. Each waveform has several harmonic
// limits so high notes do not use the full-band table and alias as aggressively.
class WavetableBank final
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int waveformCount = 4;
    static constexpr int levelCount = 9;

    WavetableBank();

    float getSample (int waveform,
                     float morph,
                     double phaseCycles,
                     double frequency,
                     double sampleRate) const noexcept;

    static const juce::StringArray& getWaveformNames();

private:
    static constexpr std::array<int, levelCount> harmonicLimits
        { 1, 2, 4, 8, 16, 32, 64, 128, 256 };

    using Table = std::array<float, tableSize + 1>;
    std::array<std::array<Table, levelCount>, waveformCount> tables {};

    void buildTables();
    static float makeWaveSample (int waveform, int harmonics, double phase) noexcept;
    float readTable (int waveform, int level, double phaseCycles) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableBank)
};

class HybridSynthVoice final : public juce::SynthesiserVoice
{
public:
    HybridSynthVoice();

    bool canPlaySound (juce::SynthesiserSound* sound) override;

    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* sound,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    // Called from the audio thread. No memory allocation is performed here.
    void setEnvelopeParameters (float attackSeconds,
                                float decaySeconds,
                                float sustainLevel,
                                float releaseSeconds);

    void setSourceParameters (const WavetableBank* bank,
                              int waveform,
                              float morph,
                              float wavetableLevel,
                              float sampleLevel,
                              float sampleStart,
                              float sampleEnd,
                              int sampleRootNote,
                              bool sampleLoop,
                              std::shared_ptr<const SampleData> sample);

    void prepare (double sampleRate, int samplesPerBlock, int outputChannels);
    void hardReset();

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override;

private:
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParameters;

    const WavetableBank* wavetableBank = nullptr;
    std::shared_ptr<const SampleData> sampleData;

    double currentSampleRate = 44100.0;
    double phaseCycles = 0.0;
    double phaseIncrement = 0.0;
    double samplePosition = 0.0;
    double sampleIncrement = 1.0;
    float velocityLevel = 0.0f;

    int waveform = 0;
    int sampleRootNote = 60;
    float wavetableMorph = 0.0f;
    float wavetableLevel = 0.7f;
    float sampleLevel = 0.0f;
    float sampleStart = 0.0f;
    float sampleEnd = 1.0f;
    bool sampleLoop = true;
    bool samplePlaying = false;

    bool hardStopPending = false;

    void setFrequencyFromNote (int midiNoteNumber);
    float getSampleValue (int outputChannel) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HybridSynthVoice)
};
