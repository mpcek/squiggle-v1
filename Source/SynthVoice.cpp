#include "SynthVoice.h"

#include <cmath>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;
}

WavetableBank::WavetableBank()
{
    buildTables();
}

const juce::StringArray& WavetableBank::getWaveformNames()
{
    static const juce::StringArray names { "Sine", "Saw", "Square", "Triangle" };
    return names;
}

float WavetableBank::makeWaveSample (int type, int harmonics, double phase) noexcept
{
    const auto p = phase * twoPi;

    if (type == 0)
        return static_cast<float> (std::sin (p));

    double value = 0.0;
    const int limit = juce::jmax (1, harmonics);

    if (type == 1) // Saw: all harmonics, alternating signs.
    {
        for (int h = 1; h <= limit; ++h)
            value += ((h & 1) != 0 ? 1.0 : -1.0) * std::sin (p * h) / h;
        value *= 2.0 / juce::MathConstants<double>::pi;
    }
    else if (type == 2) // Square: odd harmonics.
    {
        for (int h = 1; h <= limit; h += 2)
            value += std::sin (p * h) / h;
        value *= 4.0 / juce::MathConstants<double>::pi;
    }
    else // Triangle: odd harmonics with alternating signs and 1/h^2 roll-off.
    {
        for (int h = 1; h <= limit; h += 2)
        {
            const auto sign = ((h % 4) == 1) ? 1.0 : -1.0;
            value += sign * std::sin (p * h) / (static_cast<double> (h) * h);
        }
        value *= 8.0 / (juce::MathConstants<double>::pi * juce::MathConstants<double>::pi);
    }

    return static_cast<float> (value);
}

void WavetableBank::buildTables()
{
    for (int waveformIndex = 0; waveformIndex < waveformCount; ++waveformIndex)
    {
        for (int level = 0; level < levelCount; ++level)
        {
            auto& table = tables[(size_t) waveformIndex][(size_t) level];
            const auto harmonics = harmonicLimits[(size_t) level];
            float peak = 0.0f;

            for (int i = 0; i < tableSize; ++i)
            {
                const auto phase = static_cast<double> (i) / tableSize;
                table[(size_t) i] = makeWaveSample (waveformIndex, harmonics, phase);
                peak = juce::jmax (peak, std::abs (table[(size_t) i]));
            }

            const auto normalise = peak > 0.00001f ? 1.0f / peak : 1.0f;
            for (int i = 0; i < tableSize; ++i)
                table[(size_t) i] *= normalise;

            table[(size_t) tableSize] = table[0];
        }
    }
}

float WavetableBank::readTable (int waveformIndex, int level, double phaseCycles) const noexcept
{
    const auto wrapped = phaseCycles - std::floor (phaseCycles);
    const auto position = wrapped * tableSize;
    const auto index = juce::jlimit (0, tableSize - 1, (int) position);
    const auto fraction = static_cast<float> (position - index);
    const auto& table = tables[(size_t) juce::jlimit (0, waveformCount - 1, waveformIndex)]
                              [(size_t) juce::jlimit (0, levelCount - 1, level)];
    return table[(size_t) index] + (table[(size_t) index + 1] - table[(size_t) index]) * fraction;
}

float WavetableBank::getSample (int waveformIndex,
                                float morph,
                                double phaseCycles,
                                double frequency,
                                double sampleRate) const noexcept
{
    const auto safeRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    const auto safeFrequency = juce::jmax (1.0, frequency);
    const auto maxHarmonics = juce::jmax (1, (int) (safeRate / (2.0 * safeFrequency)));

    int level = 0;
    for (int i = 0; i < levelCount; ++i)
        if (harmonicLimits[(size_t) i] <= maxHarmonics)
            level = i;

    const auto first = juce::jlimit (0, waveformCount - 1, waveformIndex);
    const auto second = (first + 1) % waveformCount;
    const auto a = readTable (first, level, phaseCycles);
    const auto b = readTable (second, level, phaseCycles);
    return a + (b - a) * juce::jlimit (0.0f, 1.0f, morph);
}

HybridSynthVoice::HybridSynthVoice()
{
    envelopeParameters.attack = 0.01f;
    envelopeParameters.decay = 0.15f;
    envelopeParameters.sustain = 0.75f;
    envelopeParameters.release = 0.25f;
    envelope.setParameters (envelopeParameters);
}

bool HybridSynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<HybridSynthSound*> (sound) != nullptr;
}

void HybridSynthVoice::prepare (double sampleRate, int, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    envelope.setSampleRate (currentSampleRate);
    envelope.setParameters (envelopeParameters);
    hardReset();
}

void HybridSynthVoice::hardReset()
{
    envelope.reset();
    hardStopPending = false;
    samplePlaying = false;
    clearCurrentNote();
}

void HybridSynthVoice::setEnvelopeParameters (float attackSeconds,
                                              float decaySeconds,
                                              float sustainLevel,
                                              float releaseSeconds)
{
    envelopeParameters.attack = juce::jlimit (0.001f, 5.0f, attackSeconds);
    envelopeParameters.decay = juce::jlimit (0.001f, 5.0f, decaySeconds);
    envelopeParameters.sustain = juce::jlimit (0.0f, 1.0f, sustainLevel);
    envelopeParameters.release = juce::jlimit (0.001f, 8.0f, releaseSeconds);
    envelope.setParameters (envelopeParameters);
}

void HybridSynthVoice::setSourceParameters (const WavetableBank* bank,
                                             int newWaveform,
                                             float morph,
                                             float newWavetableLevel,
                                             float newSampleLevel,
                                             float newSampleStart,
                                             float newSampleEnd,
                                             int newSampleRootNote,
                                             bool newSampleLoop,
                                             std::shared_ptr<const SampleData> sample)
{
    wavetableBank = bank;
    waveform = juce::jlimit (0, WavetableBank::waveformCount - 1, newWaveform);
    wavetableMorph = juce::jlimit (0.0f, 1.0f, morph);
    wavetableLevel = juce::jlimit (0.0f, 1.0f, newWavetableLevel);
    sampleLevel = juce::jlimit (0.0f, 1.0f, newSampleLevel);
    sampleStart = juce::jlimit (0.0f, 0.98f, newSampleStart);
    sampleEnd = juce::jlimit (sampleStart + 0.01f, 1.0f, newSampleEnd);
    sampleRootNote = juce::jlimit (0, 127, newSampleRootNote);
    sampleLoop = newSampleLoop;

    if (sample != sampleData)
    {
        sampleData = std::move (sample);
        if (sampleData != nullptr && isVoiceActive())
        {
            const auto total = sampleData->audio.getNumSamples();
            samplePosition = sampleStart * juce::jmax (0, total - 1);
            samplePlaying = total > 1 && sampleLevel > 0.0001f;
            if (getCurrentlyPlayingNote() >= 0)
                sampleIncrement = (sampleData->sampleRate / currentSampleRate)
                    * std::pow (2.0, (getCurrentlyPlayingNote() - sampleRootNote) / 12.0);
        }
        else if (sampleData == nullptr)
        {
            samplePlaying = false;
        }
    }
}

void HybridSynthVoice::setFrequencyFromNote (int midiNoteNumber)
{
    const auto frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phaseIncrement = frequency / currentSampleRate;

    if (sampleData != nullptr)
    {
        sampleIncrement = (sampleData->sampleRate / currentSampleRate)
            * std::pow (2.0, (midiNoteNumber - sampleRootNote) / 12.0);
        const auto last = juce::jmax (0, sampleData->audio.getNumSamples() - 1);
        samplePosition = sampleStart * last;
        samplePlaying = last > 0 && sampleLevel > 0.0001f;
    }
    else
    {
        samplePlaying = false;
    }
}

void HybridSynthVoice::startNote (int midiNoteNumber,
                                  float velocity,
                                  juce::SynthesiserSound*,
                                  int)
{
    velocityLevel = velocity;
    setFrequencyFromNote (midiNoteNumber);
    hardStopPending = false;
    envelope.setParameters (envelopeParameters);
    envelope.noteOn();
}

void HybridSynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        envelope.noteOff();
    }
    else
    {
        hardStopPending = true;
        samplePlaying = false;
        clearCurrentNote();
    }
}

void HybridSynthVoice::pitchWheelMoved (int)
{
}

void HybridSynthVoice::controllerMoved (int, int)
{
}

float HybridSynthVoice::getSampleValue (int outputChannel) noexcept
{
    if (! samplePlaying || sampleData == nullptr || sampleData->audio.getNumSamples() < 2)
        return 0.0f;

    const auto totalSamples = sampleData->audio.getNumSamples();
    const auto startIndex = juce::jlimit (0, totalSamples - 1,
                                          (int) (sampleStart * (totalSamples - 1)));
    const auto endIndex = juce::jlimit (startIndex + 1, totalSamples - 1,
                                        (int) (sampleEnd * (totalSamples - 1)));

    if (samplePosition < startIndex)
        samplePosition = startIndex;

    if (samplePosition >= endIndex)
    {
        if (sampleLoop)
        {
            const auto length = juce::jmax (1.0, (double) (endIndex - startIndex));
            samplePosition = startIndex + std::fmod (samplePosition - startIndex, length);
        }
        else
        {
            samplePlaying = false;
            return 0.0f;
        }
    }

    const auto index = juce::jlimit (startIndex, endIndex - 1, (int) samplePosition);
    const auto nextIndex = juce::jmin (endIndex, index + 1);
    const auto fraction = static_cast<float> (samplePosition - index);
    const auto channel = juce::jmin (outputChannel, sampleData->audio.getNumChannels() - 1);
    const auto a = sampleData->audio.getSample (channel, index);
    const auto b = sampleData->audio.getSample (channel, nextIndex);

    return a + (b - a) * fraction;
}

void HybridSynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                        int startSample,
                                        int numSamples)
{
    if (! isVoiceActive())
    {
        if (hardStopPending)
        {
            envelope.reset();
            hardStopPending = false;
        }
        return;
    }

    const auto outputChannels = outputBuffer.getNumChannels();
    const auto frequency = juce::MidiMessage::getMidiNoteInHertz (getCurrentlyPlayingNote());
    const auto sourceLevelSum = wavetableLevel + sampleLevel;
    const auto sourceScale = sourceLevelSum > 1.0f ? 1.0f / sourceLevelSum : 1.0f;
    auto* left = outputBuffer.getWritePointer (0);
    auto* right = outputChannels > 1 ? outputBuffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto env = envelope.getNextSample();
        float oscillator = 0.0f;
        if (wavetableBank != nullptr && wavetableLevel > 0.0001f)
            oscillator = wavetableBank->getSample (waveform, wavetableMorph, phaseCycles,
                                                   frequency, currentSampleRate);

        const auto sampleL = getSampleValue (0);
        const auto sampleR = outputChannels > 1 ? getSampleValue (1) : sampleL;
        if (samplePlaying)
            samplePosition += sampleIncrement;
        const auto leftSource = (oscillator * wavetableLevel + sampleL * sampleLevel) * sourceScale;
        const auto rightSource = (oscillator * wavetableLevel + sampleR * sampleLevel) * sourceScale;
        const auto leftSample = leftSource * velocityLevel * env * 0.18f;
        const auto rightSample = rightSource * velocityLevel * env * 0.18f;

        phaseCycles += phaseIncrement;
        if (phaseCycles >= 1.0)
            phaseCycles -= std::floor (phaseCycles);

        const auto index = startSample + i;
        left[index] += leftSample;
        if (right != nullptr)
            right[index] += rightSample;

        if (! envelope.isActive())
        {
            samplePlaying = false;
            clearCurrentNote();
            break;
        }
    }
}
