#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

HybridSynthAudioProcessor::HybridSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SQUIGGLE_STATE", createParameterLayout())
{
    formatManager.registerBasicFormats();

    polyVoices.reserve (maximumVoices);
    monoVoices.reserve (1);

    for (int i = 0; i < maximumVoices; ++i)
    {
        auto* voice = new HybridSynthVoice();
        polyVoices.push_back (voice);
        polySynth.addVoice (voice);
    }

    auto* monoVoice = new HybridSynthVoice();
    monoVoices.push_back (monoVoice);
    monoSynth.addVoice (monoVoice);

    polySynth.addSound (new HybridSynthSound());
    monoSynth.addSound (new HybridSynthSound());
    polySynth.setNoteStealingEnabled (true);
    monoSynth.setNoteStealingEnabled (true);

    voiceModeParam       = parameters.getRawParameterValue ("voiceMode");
    attackParam          = parameters.getRawParameterValue ("attack");
    decayParam           = parameters.getRawParameterValue ("decay");
    sustainParam         = parameters.getRawParameterValue ("sustain");
    releaseParam         = parameters.getRawParameterValue ("release");
    masterGainParam      = parameters.getRawParameterValue ("masterGain");
    waveformParam        = parameters.getRawParameterValue ("wtWaveform");
    morphParam           = parameters.getRawParameterValue ("wtMorph");
    wavetableLevelParam  = parameters.getRawParameterValue ("wtLevel");
    sampleLevelParam     = parameters.getRawParameterValue ("sampleLevel");
    sampleStartParam     = parameters.getRawParameterValue ("sampleStart");
    sampleEndParam       = parameters.getRawParameterValue ("sampleEnd");
    sampleRootNoteParam  = parameters.getRawParameterValue ("sampleRootNote");
    sampleLoopParam      = parameters.getRawParameterValue ("sampleLoop");

    jassert (voiceModeParam && attackParam && decayParam && sustainParam && releaseParam
             && masterGainParam && waveformParam && morphParam && wavetableLevelParam
             && sampleLevelParam && sampleStartParam && sampleEndParam
             && sampleRootNoteParam && sampleLoopParam);
}

juce::AudioProcessorValueTreeState::ParameterLayout
HybridSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "voiceMode", 1 }, "Voice Mode",
        juce::StringArray { "Poly", "Mono" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decay", 1 }, "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.75f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (0.001f, 8.0f, 0.001f, 0.4f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "masterGain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -6.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "wtWaveform", 1 }, "Wavetable",
        WavetableBank::getWaveformNames(), 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wtMorph", 1 }, "Wavetable Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wtLevel", 1 }, "Wavetable Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sampleLevel", 1 }, "Sample Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sampleStart", 1 }, "Sample Start",
        juce::NormalisableRange<float> (0.0f, 0.98f, 0.001f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "sampleEnd", 1 }, "Sample End",
        juce::NormalisableRange<float> (0.02f, 1.0f, 0.001f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "sampleRootNote", 1 }, "Sample Root Note", 0, 127, 60));
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "sampleLoop", 1 }, "Sample Loop", true));

    return { params.begin(), params.end() };
}

double HybridSynthAudioProcessor::getTailLengthSeconds() const
{
    return releaseParam != nullptr ? (double) releaseParam->load() : 0.0;
}

void HybridSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    polySynth.setCurrentPlaybackSampleRate (sampleRate);
    monoSynth.setCurrentPlaybackSampleRate (sampleRate);

    const auto channels = getTotalNumOutputChannels();
    for (auto* voice : polyVoices)
        voice->prepare (sampleRate, samplesPerBlock, channels);
    for (auto* voice : monoVoices)
        voice->prepare (sampleRate, samplesPerBlock, channels);

    lastVoiceMode = voiceModeParam->load() < 0.5f ? 0 : 1;
    lastGain = juce::Decibels::decibelsToGain (masterGainParam->load());
}

void HybridSynthAudioProcessor::releaseResources()
{
}

bool HybridSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void HybridSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto numSamples = buffer.getNumSamples();
    const auto mode = voiceModeParam->load() < 0.5f ? 0 : 1;

    const auto attack = attackParam->load();
    const auto decay = decayParam->load();
    const auto sustain = sustainParam->load();
    const auto release = releaseParam->load();

    const auto waveform = (int) waveformParam->load();
    const auto morph = morphParam->load();
    const auto wtLevel = wavetableLevelParam->load();
    const auto smpLevel = sampleLevelParam->load();
    const auto smpStart = sampleStartParam->load();
    const auto smpEnd = sampleEndParam->load();
    const auto smpRoot = (int) sampleRootNoteParam->load();
    const auto smpLoop = sampleLoopParam->load() >= 0.5f;
    auto currentSample = std::atomic_load (&loadedSample);

    for (auto* voice : polyVoices)
    {
        voice->setEnvelopeParameters (attack, decay, sustain, release);
        voice->setSourceParameters (&wavetableBank, waveform, morph, wtLevel, smpLevel,
                                    smpStart, smpEnd, smpRoot, smpLoop, currentSample);
    }
    for (auto* voice : monoVoices)
    {
        voice->setEnvelopeParameters (attack, decay, sustain, release);
        voice->setSourceParameters (&wavetableBank, waveform, morph, wtLevel, smpLevel,
                                    smpStart, smpEnd, smpRoot, smpLoop, currentSample);
    }

    if (mode != lastVoiceMode)
    {
        (lastVoiceMode == 0 ? polySynth : monoSynth).allNotesOff (0, true);
        lastVoiceMode = mode;
    }

    auto& activeSynth = (mode == 0 ? polySynth : monoSynth);
    auto& inactiveSynth = (mode == 0 ? monoSynth : polySynth);

    activeSynth.renderNextBlock (buffer, midiMessages, 0, numSamples);
    juce::MidiBuffer noMidi;
    inactiveSynth.renderNextBlock (buffer, noMidi, 0, numSamples);

    const auto targetGain = juce::Decibels::decibelsToGain (masterGainParam->load());
    if (juce::approximatelyEqual (targetGain, lastGain))
        buffer.applyGain (targetGain);
    else
        buffer.applyGainRamp (0, numSamples, lastGain, targetGain);
    lastGain = targetGain;
}

juce::AudioProcessorEditor* HybridSynthAudioProcessor::createEditor()
{
    return new HybridSynthAudioProcessorEditor (*this);
}

juce::File HybridSynthAudioProcessor::getUserSamplesDirectory() const
{
    auto directory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                         .getChildFile ("SQUIGGLE")
                         .getChildFile ("UserLibrary")
                         .getChildFile ("Samples");
    return directory;
}

juce::Array<juce::File> HybridSynthAudioProcessor::getUserSampleFiles() const
{
    juce::Array<juce::File> files;
    const auto directory = getUserSamplesDirectory();
    if (directory.isDirectory())
    {
        directory.findChildFiles (files, juce::File::findFiles, false,
                                  "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    }

    std::vector<juce::File> sortedFiles;
    sortedFiles.reserve ((size_t) files.size());
    for (int i = 0; i < files.size(); ++i)
        sortedFiles.push_back (files.getReference (i));

    std::sort (sortedFiles.begin(), sortedFiles.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
    });

    juce::Array<juce::File> result;
    for (const auto& file : sortedFiles)
        result.add (file);
    return result;
}

void HybridSynthAudioProcessor::publishSample (std::shared_ptr<const SampleData> newSample)
{
    auto oldSample = std::atomic_load (&loadedSample);
    if (oldSample != nullptr)
        retiredSamples.push_back (std::move (oldSample));
    std::atomic_store (&loadedSample, std::move (newSample));

    retiredSamples.erase (std::remove_if (retiredSamples.begin(), retiredSamples.end(),
        [] (const std::shared_ptr<const SampleData>& sample) { return sample.use_count() == 1; }),
        retiredSamples.end());
}

bool HybridSynthAudioProcessor::loadSampleFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 2 || reader->sampleRate <= 0.0)
        return false;

    constexpr double maximumSampleSeconds = 120.0;
    const auto maximumSamples = static_cast<decltype (reader->lengthInSamples)> (
        reader->sampleRate * maximumSampleSeconds);
    const auto samplesToRead = (int) juce::jmin (reader->lengthInSamples, maximumSamples);
    const auto channelsToRead = (int) juce::jlimit (1u, 2u, reader->numChannels);

    auto data = std::make_shared<SampleData>();
    data->audio.setSize (channelsToRead, samplesToRead);
    data->sampleRate = reader->sampleRate;
    data->file = file;
    data->name = file.getFileName();

    if (! reader->read (&data->audio, 0, samplesToRead, 0, true, true))
        return false;

    publishSample (std::move (data));
    parameters.state.setProperty ("samplePath", file.getFullPathName(), nullptr);
    return true;
}

bool HybridSynthAudioProcessor::importSampleToLibrary (const juce::File& sourceFile)
{
    if (! sourceFile.existsAsFile())
        return false;

    auto directory = getUserSamplesDirectory();
    if (! directory.isDirectory() && directory.createDirectory().failed())
        return false;

    auto destination = directory.getChildFile (sourceFile.getFileName());
    if (destination == sourceFile)
        return loadSampleFromFile (sourceFile);

    if (destination.existsAsFile())
        destination = directory.getNonexistentChildFile (sourceFile.getFileNameWithoutExtension(),
                                                          sourceFile.getFileExtension(), false);

    if (! sourceFile.copyFileTo (destination))
        return false;

    return loadSampleFromFile (destination);
}

bool HybridSynthAudioProcessor::deleteUserSample (const juce::File& file)
{
    const auto libraryDirectory = getUserSamplesDirectory();
    if (! file.existsAsFile() || file.getParentDirectory() != libraryDirectory)
        return false;

    const auto loadedPath = getLoadedSamplePath();
    if (loadedPath == file.getFullPathName())
        clearSample();

    return file.deleteFile();
}

void HybridSynthAudioProcessor::clearSample()
{
    publishSample (nullptr);
    parameters.state.setProperty ("samplePath", juce::String(), nullptr);
}

juce::String HybridSynthAudioProcessor::getLoadedSampleName() const
{
    const auto sample = std::atomic_load (&loadedSample);
    return sample != nullptr ? sample->name : juce::String ("No sample loaded");
}

juce::String HybridSynthAudioProcessor::getLoadedSamplePath() const
{
    const auto sample = std::atomic_load (&loadedSample);
    return sample != nullptr ? sample->file.getFullPathName() : juce::String();
}

std::shared_ptr<const SampleData> HybridSynthAudioProcessor::getLoadedSampleData() const
{
    return std::atomic_load (&loadedSample);
}

void HybridSynthAudioProcessor::setParameterPlainValue (const juce::String& parameterID,
                                                        float plainValue)
{
    if (auto* parameter = parameters.getParameter (parameterID))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plainValue));
}

void HybridSynthAudioProcessor::randomizeEnvelope()
{
    auto& random = juce::Random::getSystemRandom();
    setParameterPlainValue ("attack", random.nextFloat() < 0.45f
        ? random.nextFloat() * 0.25f : 0.25f + random.nextFloat() * 2.5f);
    setParameterPlainValue ("decay", 0.03f + random.nextFloat() * 2.0f);
    setParameterPlainValue ("sustain", random.nextFloat());
    setParameterPlainValue ("release", 0.03f + random.nextFloat() * 3.5f);
}

void HybridSynthAudioProcessor::randomizeWavetable()
{
    auto& random = juce::Random::getSystemRandom();
    setParameterPlainValue ("wtWaveform", (float) random.nextInt (WavetableBank::waveformCount));
    setParameterPlainValue ("wtMorph", random.nextFloat());
    setParameterPlainValue ("wtLevel", 0.25f + random.nextFloat() * 0.75f);
}

void HybridSynthAudioProcessor::randomizeSample()
{
    auto& random = juce::Random::getSystemRandom();
    const auto start = random.nextFloat() * 0.72f;
    const auto remaining = 1.0f - start;
    const auto end = start + 0.12f + random.nextFloat() * juce::jmax (0.0f, remaining - 0.12f);
    setParameterPlainValue ("sampleStart", start);
    setParameterPlainValue ("sampleEnd", juce::jmin (1.0f, end));
    setParameterPlainValue ("sampleLevel", 0.2f + random.nextFloat() * 0.8f);
    setParameterPlainValue ("sampleLoop", random.nextBool() ? 1.0f : 0.0f);
}

void HybridSynthAudioProcessor::randomizeAll()
{
    randomizeEnvelope();
    randomizeWavetable();
    randomizeSample();
    auto& random = juce::Random::getSystemRandom();
    setParameterPlainValue ("masterGain", -12.0f + random.nextFloat() * 6.0f);
}

void HybridSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    const auto sample = std::atomic_load (&loadedSample);
    state.setProperty ("samplePath", sample != nullptr ? sample->file.getFullPathName() : juce::String(), nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void HybridSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    auto restoredState = juce::ValueTree::fromXml (*xml);
    const auto samplePath = restoredState.getProperty ("samplePath").toString();
    parameters.replaceState (restoredState);

    if (samplePath.isNotEmpty())
    {
        if (! loadSampleFromFile (juce::File (samplePath)))
            clearSample();
    }
    else
    {
        clearSample();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HybridSynthAudioProcessor();
}
