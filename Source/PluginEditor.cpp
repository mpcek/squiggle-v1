#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

namespace
{
    const juce::Colour backgroundColour (0xff1c2023);
    const juce::Colour panelColour (0xff282e32);
    const juce::Colour panelEdgeColour (0xff3a4247);
    const juce::Colour accentColour (0xff79b7b0);
    const juce::Colour textColour (0xfff1f3f2);
    const juce::Colour mutedTextColour (0xffaeb8b8);
}

void EnvelopeDisplay::setSliders (juce::Slider& attack, juce::Slider& decay,
                                  juce::Slider& sustain, juce::Slider& release)
{
    attackSlider = &attack;
    decaySlider = &decay;
    sustainSlider = &sustain;
    releaseSlider = &release;
}

juce::Rectangle<float> EnvelopeDisplay::getPlotBounds() const
{
    return getLocalBounds().toFloat().reduced (18.0f, 18.0f);
}

std::array<juce::Point<float>, 5> EnvelopeDisplay::getEnvelopePoints() const
{
    const auto plot = getPlotBounds();
    const auto attack = attackSlider != nullptr ? (float) attackSlider->getValue() : 0.01f;
    const auto decay = decaySlider != nullptr ? (float) decaySlider->getValue() : 0.15f;
    const auto sustain = sustainSlider != nullptr ? (float) sustainSlider->getValue() : 0.75f;
    const auto release = releaseSlider != nullptr ? (float) releaseSlider->getValue() : 0.25f;

    // Sustain is held for a fixed visual reference interval. Attack, decay and
    // release are drawn in proportion to their real parameter times.
    constexpr float visualHoldSeconds = 0.25f;
    const auto total = juce::jmax (0.001f, attack + decay + visualHoldSeconds + release);
    const auto x1 = plot.getX() + plot.getWidth() * attack / total;
    const auto x2 = plot.getX() + plot.getWidth() * (attack + decay) / total;
    const auto x3 = plot.getX() + plot.getWidth() * (attack + decay + visualHoldSeconds) / total;
    const auto ySustain = plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, sustain);

    return
    {{
        { plot.getX(), plot.getBottom() },
        { x1, plot.getY() },
        { x2, ySustain },
        { x3, ySustain },
        { plot.getRight(), plot.getBottom() }
    }};
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff202629));
    g.fillRoundedRectangle (bounds, 8.0f);

    const auto plot = getPlotBounds();
    g.setColour (juce::Colour (0xff3c484d));
    for (int i = 1; i < 4; ++i)
    {
        const auto y = plot.getY() + plot.getHeight() * (float) i / 4.0f;
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
    }
    for (int i = 1; i < 8; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (float) i / 8.0f;
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }

    const auto points = getEnvelopePoints();
    juce::Path path;
    path.startNewSubPath (points[0]);
    path.lineTo (points[1]);
    path.lineTo (points[2]);
    path.lineTo (points[3]);
    path.lineTo (points[4]);

    g.setColour (accentColour.withAlpha (0.16f));
    juce::Path filled = path;
    filled.lineTo (plot.getRight(), plot.getBottom());
    filled.lineTo (plot.getX(), plot.getBottom());
    filled.closeSubPath();
    g.fillPath (filled);

    g.setColour (accentColour);
    g.strokePath (path, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xffe4b86a));
    g.drawVerticalLine ((int) points[3].x, plot.getY(), plot.getBottom());

    const auto sustainMidX = (points[2].x + points[3].x) * 0.5f;
    const auto sustainHandleCandidate = points[2].y < plot.getCentreY()
        ? points[2].y + 24.0f : points[2].y - 24.0f;
    const auto sustainHandleY = juce::jlimit (plot.getY() + 8.0f,
                                               plot.getBottom() - 8.0f,
                                               sustainHandleCandidate);
    const juce::Point<float> sustainHandle (sustainMidX, sustainHandleY);
    g.setColour (mutedTextColour);
    g.drawLine (sustainMidX, points[2].y, sustainHandle.x, sustainHandle.y, 1.0f);

    const std::array<juce::Point<float>, 4> handles
        { points[1], points[2], sustainHandle, points[3] };
    for (const auto& point : handles)
    {
        g.setColour (juce::Colour (0xff101719));
        g.fillEllipse (point.x - 6.0f, point.y - 6.0f, 12.0f, 12.0f);
        g.setColour (textColour);
        g.drawEllipse (point.x - 5.0f, point.y - 5.0f, 10.0f, 10.0f, 1.5f);
    }

    g.setColour (mutedTextColour);
    g.setFont (11.0f);
    g.drawText ("NOTE ON", (int) plot.getX(), (int) plot.getBottom() + 2, 54, 14,
                juce::Justification::left);
    g.drawText ("NOTE OFF", (int) points[3].x - 32, (int) plot.getBottom() + 2, 64, 14,
                juce::Justification::centred);
    g.drawText ("TIME →", (int) plot.getRight() - 48, (int) plot.getBottom() + 2, 48, 14,
                juce::Justification::right);
}

void EnvelopeDisplay::mouseDown (const juce::MouseEvent& event)
{
    const auto points = getEnvelopePoints();
    const auto plot = getPlotBounds();
    const auto sustainMidX = (points[2].x + points[3].x) * 0.5f;
    const auto sustainHandleCandidate = points[2].y < plot.getCentreY()
        ? points[2].y + 24.0f : points[2].y - 24.0f;
    const auto sustainHandleY = juce::jlimit (plot.getY() + 8.0f,
                                               plot.getBottom() - 8.0f,
                                               sustainHandleCandidate);
    const juce::Point<float> sustainHandle (sustainMidX, sustainHandleY);
    const std::array<juce::Point<float>, 4> handles
        { points[1], points[2], sustainHandle, points[3] };
    const std::array<Handle, 4> handleTypes
        { Handle::attack, Handle::decay, Handle::sustain, Handle::release };

    auto closestDistance = 22.0f;
    draggingHandle = Handle::none;
    for (size_t i = 0; i < handles.size(); ++i)
    {
        const auto distance = handles[i].getDistanceFrom (event.position);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            draggingHandle = handleTypes[i];
        }
    }
}

void EnvelopeDisplay::setSliderValue (juce::Slider* slider, double value)
{
    if (slider != nullptr)
        slider->setValue (value, juce::sendNotificationSync);
}

void EnvelopeDisplay::mouseDrag (const juce::MouseEvent& event)
{
    if (draggingHandle == Handle::none || attackSlider == nullptr || decaySlider == nullptr
        || sustainSlider == nullptr || releaseSlider == nullptr)
        return;

    const auto plot = getPlotBounds();
    const auto x = juce::jlimit (0.015f, 0.985f,
        (event.position.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
    const auto y = juce::jlimit (0.0f, 1.0f,
        1.0f - (event.position.y - plot.getY()) / juce::jmax (1.0f, plot.getHeight()));

    const auto attack = (float) attackSlider->getValue();
    const auto decay = (float) decaySlider->getValue();
    const auto release = (float) releaseSlider->getValue();
    constexpr float visualHoldSeconds = 0.25f;

    if (draggingHandle == Handle::attack)
    {
        const auto newAttack = x * (decay + visualHoldSeconds + release) / (1.0f - x);
        setSliderValue (attackSlider, juce::jlimit (0.001f, 5.0f, newAttack));
    }
    else if (draggingHandle == Handle::decay)
    {
        const auto newDecay = x * (visualHoldSeconds + release) / (1.0f - x) - attack;
        setSliderValue (decaySlider, juce::jlimit (0.001f, 5.0f, newDecay));
    }
    else if (draggingHandle == Handle::sustain)
    {
        setSliderValue (sustainSlider, y);
    }
    else if (draggingHandle == Handle::release)
    {
        const auto beforeRelease = attack + decay + visualHoldSeconds;
        const auto newRelease = beforeRelease * (1.0f - x) / x;
        setSliderValue (releaseSlider, juce::jlimit (0.001f, 8.0f, newRelease));
    }

    repaint();
}

float WaveformDisplay::waveformValue (int type, float phase) noexcept
{
    const auto p = phase - std::floor (phase);
    switch (type)
    {
        case 1: return 2.0f * p - 1.0f;
        case 2: return p < 0.5f ? 1.0f : -1.0f;
        case 3: return 1.0f - 4.0f * std::abs (p - 0.5f);
        default: return std::sin (p * juce::MathConstants<float>::twoPi);
    }
}

void WaveformDisplay::setWaveform (int newWaveform, float newMorph)
{
    waveform = juce::jlimit (0, 3, newWaveform);
    morph = juce::jlimit (0.0f, 1.0f, newMorph);
    repaint();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff202629));
    g.fillRoundedRectangle (bounds, 8.0f);

    const auto centreY = bounds.getCentreY();
    g.setColour (juce::Colour (0xff465258));
    g.drawHorizontalLine ((int) centreY, bounds.getX() + 12.0f, bounds.getRight() - 12.0f);

    const auto nextWaveform = (waveform + 1) % 4;
    juce::Path path;
    const auto usableWidth = juce::jmax (1.0f, bounds.getWidth() - 24.0f);
    const auto amplitude = bounds.getHeight() * 0.37f;
    const auto left = bounds.getX() + 12.0f;

    for (int i = 0; i <= 256; ++i)
    {
        const auto phase = (float) i / 256.0f;
        const auto a = waveformValue (waveform, phase);
        const auto b = waveformValue (nextWaveform, phase);
        const auto value = a + (b - a) * morph;
        const auto point = juce::Point<float> (left + usableWidth * phase,
                                               centreY - value * amplitude);
        if (i == 0)
            path.startNewSubPath (point);
        else
            path.lineTo (point);
    }

    g.setColour (accentColour);
    g.strokePath (path, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
}


SampleWaveformDisplay::SampleWaveformDisplay (HybridSynthAudioProcessor& processor)
    : audioProcessor (processor)
{
}

void SampleWaveformDisplay::setRangeSliders (juce::Slider& start, juce::Slider& end)
{
    startSlider = &start;
    endSlider = &end;
}

void SampleWaveformDisplay::drawWaveform (juce::Graphics& g,
                                          const juce::AudioBuffer<float>& audio,
                                          float start,
                                          float end,
                                          juce::Colour colour,
                                          juce::Rectangle<float> plot) const
{
    const auto sampleCount = audio.getNumSamples();
    if (sampleCount < 2 || audio.getNumChannels() < 1 || plot.getWidth() < 2.0f)
        return;

    const auto firstSample = juce::jlimit (0, sampleCount - 1,
        (int) (juce::jlimit (0.0f, 1.0f, start) * (sampleCount - 1)));
    const auto lastSample = juce::jlimit (firstSample + 1, sampleCount - 1,
        (int) (juce::jlimit (0.0f, 1.0f, end) * (sampleCount - 1)));
    const auto columns = juce::jmax (1, (int) plot.getWidth());
    const auto centreY = plot.getCentreY();
    const auto amplitude = plot.getHeight() * 0.44f;
    const auto* data = audio.getReadPointer (0);

    g.setColour (colour);
    for (int x = 0; x < columns; ++x)
    {
        const auto fraction0 = (float) x / (float) columns;
        const auto fraction1 = (float) (x + 1) / (float) columns;
        const auto rangeStart = firstSample + (int) (fraction0 * (lastSample - firstSample));
        const auto rangeEnd = juce::jmax (rangeStart + 1,
            firstSample + (int) (fraction1 * (lastSample - firstSample)));
        const auto safeEnd = juce::jmin (lastSample, rangeEnd);

        float minimum = 1.0f;
        float maximum = -1.0f;
        for (int i = rangeStart; i <= safeEnd; ++i)
        {
            minimum = juce::jmin (minimum, data[i]);
            maximum = juce::jmax (maximum, data[i]);
        }

        const auto px = plot.getX() + (float) x;
        g.drawLine (px, centreY - maximum * amplitude,
                    px, centreY - minimum * amplitude, 1.0f);
    }
}

void SampleWaveformDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff202629));
    g.fillRoundedRectangle (bounds, 7.0f);

    const auto plot = bounds.reduced (8.0f, 5.0f);
    const auto sample = audioProcessor.getLoadedSampleData();
    if (sample == nullptr || sample->audio.getNumSamples() < 2)
    {
        g.setColour (mutedTextColour);
        g.setFont (11.0f);
        g.drawText ("IMPORT A SAMPLE TO VIEW ITS WAVEFORM", bounds.toNearestInt(),
                    juce::Justification::centred);
        return;
    }

    g.setColour (juce::Colour (0xff465258));
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
    drawWaveform (g, sample->audio, 0.0f, 1.0f, juce::Colour (0xff657176), plot);

    const auto start = juce::jlimit (0.0f, 0.98f,
        startSlider != nullptr ? (float) startSlider->getValue() : 0.0f);
    const auto end = juce::jlimit (start + 0.01f, 1.0f,
        endSlider != nullptr ? (float) endSlider->getValue() : 1.0f);
    const auto selectionLeft = plot.getX() + plot.getWidth() * start;
    const auto selectionRight = plot.getX() + plot.getWidth() * end;
    g.setColour (accentColour.withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float> (selectionLeft, plot.getY(),
                                       juce::jmax (1.0f, selectionRight - selectionLeft),
                                       plot.getHeight()));
    auto selectionPlot = plot.withLeft (selectionLeft).withRight (selectionRight);
    drawWaveform (g, sample->audio, start, end, accentColour, selectionPlot);
    g.setColour (juce::Colour (0xffe4b86a));
    g.drawVerticalLine ((int) selectionLeft, plot.getY(), plot.getBottom());
    g.drawVerticalLine ((int) selectionRight, plot.getY(), plot.getBottom());
}

HybridSynthAudioProcessorEditor::HybridSynthAudioProcessorEditor (HybridSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), sampleWaveformDisplay (audioProcessor)
{
    setSize (1080, 760);
    setResizable (true, true);
    setResizeLimits (900, 700, 1600, 1100);

    titleLabel.setText ("SQUIGGLE!", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (25.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (titleLabel);

    for (auto* tab : { &synthTab, &userTab })
    {
        tab->setColour (juce::TextButton::buttonColourId, panelColour);
        tab->setColour (juce::TextButton::textColourOffId, textColour);
        addAndMakeVisible (*tab);
    }
    synthTab.onClick = [this] { showTab (false); };
    userTab.onClick = [this] { showTab (true); };

    settingsButton.setTooltip ("Technical settings");
    settingsButton.setColour (juce::TextButton::buttonColourId, panelColour);
    settingsButton.setColour (juce::TextButton::textColourOffId, textColour);
    settingsButton.onClick = [this] { showTechnicalSettingsMessage(); };
    addAndMakeVisible (settingsButton);

    configureLabel (voiceModeLabel, "VOICE MODE");
    voiceModeBox.addItem ("POLY", 1);
    voiceModeBox.addItem ("MONO", 2);
    addAndMakeVisible (voiceModeBox);
    voiceModeAttachment = std::make_unique<ComboAttachment> (
        audioProcessor.parameters, "voiceMode", voiceModeBox);

    configureLabel (sourceTitle, "SOURCE / WAVETABLE");
    waveformBox.addItemList (WavetableBank::getWaveformNames(), 1);
    addAndMakeVisible (waveformBox);
    waveformAttachment = std::make_unique<ComboAttachment> (
        audioProcessor.parameters, "wtWaveform", waveformBox);
    addAndMakeVisible (waveformDisplay);

    configureSlider (morphSlider, morphLabel, "MORPH");
    configureSlider (wavetableLevelSlider, wavetableLevelLabel, "WT LEVEL");
    morphAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "wtMorph", morphSlider);
    wavetableLevelAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "wtLevel", wavetableLevelSlider);

    morphSlider.onValueChange = [this]
    {
        waveformDisplay.setWaveform (waveformBox.getSelectedId() - 1, (float) morphSlider.getValue());
    };
    waveformBox.onChange = [this]
    {
        waveformDisplay.setWaveform (waveformBox.getSelectedId() - 1, (float) morphSlider.getValue());
    };

    configureLabel (sampleTitle, "SAMPLE");
    importSampleButton.onClick = [this] { openSampleChooser(); };
    importSampleButton.setTooltip ("Import an audio file into the local SQUIGGLE! sample library");
    addAndMakeVisible (importSampleButton);
    configureLabel (sampleNameLabel, audioProcessor.getLoadedSampleName());
    sampleWaveformDisplay.setRangeSliders (sampleStartSlider, sampleEndSlider);
    addAndMakeVisible (sampleWaveformDisplay);

    configureSlider (sampleLevelSlider, sampleLevelLabel, "SAMPLE LEVEL");
    configureSlider (sampleStartSlider, sampleStartLabel, "START");
    configureSlider (sampleEndSlider, sampleEndLabel, "END");
    configureSlider (sampleRootSlider, sampleRootLabel, "ROOT NOTE");
    sampleLevelAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "sampleLevel", sampleLevelSlider);
    sampleStartAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "sampleStart", sampleStartSlider);
    sampleEndAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "sampleEnd", sampleEndSlider);
    sampleRootAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "sampleRootNote", sampleRootSlider);
    sampleStartSlider.textFromValueFunction = [] (double value)
        { return juce::String (value * 100.0, 1) + " %"; };
    sampleStartSlider.valueFromTextFunction = [] (const juce::String& text)
        { return text.trim().replace ("%", "").getDoubleValue() / 100.0; };
    sampleEndSlider.textFromValueFunction = [] (double value)
        { return juce::String (value * 100.0, 1) + " %"; };
    sampleEndSlider.valueFromTextFunction = [] (const juce::String& text)
        { return text.trim().replace ("%", "").getDoubleValue() / 100.0; };
    sampleRootSlider.textFromValueFunction = [] (double value)
        { return juce::String ((int) value); };

    sampleLoopButton.setColour (juce::ToggleButton::textColourId, textColour);
    addAndMakeVisible (sampleLoopButton);
    sampleLoopAttachment = std::make_unique<ButtonAttachment> (
        audioProcessor.parameters, "sampleLoop", sampleLoopButton);

    configureLabel (envelopeTitle, "AMPLITUDE ENVELOPE");
    envelopeDisplay.setSliders (attackSlider, decaySlider, sustainSlider, releaseSlider);
    addAndMakeVisible (envelopeDisplay);
    configureSlider (attackSlider, attackLabel, "ATTACK");
    configureSlider (decaySlider, decayLabel, "DECAY");
    configureSlider (sustainSlider, sustainLabel, "SUSTAIN");
    configureSlider (releaseSlider, releaseLabel, "RELEASE");
    attackAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "attack", attackSlider);
    decayAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "decay", decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "release", releaseSlider);

    for (auto* envelopeSlider : { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider })
        envelopeSlider->onValueChange = [this] { envelopeDisplay.repaint(); };

    sampleStartSlider.onValueChange = [this]
    {
        const auto start = sampleStartSlider.getValue();
        if (start + 0.01 > sampleEndSlider.getValue() + 0.0001)
            sampleEndSlider.setValue (juce::jmin (1.0, start + 0.01), juce::sendNotificationSync);
        sampleWaveformDisplay.repaint();
    };
    sampleEndSlider.onValueChange = [this]
    {
        const auto end = sampleEndSlider.getValue();
        if (end - 0.01 < sampleStartSlider.getValue() - 0.0001)
            sampleStartSlider.setValue (juce::jmax (0.0, end - 0.01), juce::sendNotificationSync);
        sampleWaveformDisplay.repaint();
    };

    for (auto* timeSlider : { &attackSlider, &decaySlider, &releaseSlider })
    {
        timeSlider->textFromValueFunction = [] (double value)
        {
            return value < 1.0 ? juce::String (value * 1000.0, 0) + " ms"
                               : juce::String (value, 2) + " s";
        };
        timeSlider->valueFromTextFunction = [] (const juce::String& input)
        {
            auto text = input.trim().toLowerCase();
            if (text.endsWith ("ms"))
                return text.dropLastCharacters (2).getDoubleValue() / 1000.0;
            if (text.endsWith ("s"))
                return text.dropLastCharacters (1).getDoubleValue();
            return text.getDoubleValue();
        };
    }

    configureSlider (masterGainSlider, masterGainLabel, "MASTER GAIN");
    masterGainAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "masterGain", masterGainSlider);

    randomizeEnvelopeButton.onClick = [this] { audioProcessor.randomizeEnvelope(); };
    randomizeWavetableButton.onClick = [this] { audioProcessor.randomizeWavetable(); };
    randomizeSampleButton.onClick = [this] { audioProcessor.randomizeSample(); };
    randomizeAllButton.onClick = [this] { audioProcessor.randomizeAll(); };
    addAndMakeVisible (randomizeEnvelopeButton);
    addAndMakeVisible (randomizeWavetableButton);
    addAndMakeVisible (randomizeSampleButton);
    addAndMakeVisible (randomizeAllButton);

    configureLabel (userTitle, "USER LIBRARY");
    configureLabel (userDescription, "Samples imported here are copied to your local SQUIGGLE! library.");
    addAndMakeVisible (librarySamplesBox);
    refreshLibraryButton.onClick = [this] { refreshLibraryList(); };
    loadLibrarySampleButton.onClick = [this]
    {
        const auto file = getSelectedLibraryFile();
        if (file.existsAsFile())
            audioProcessor.loadSampleFromFile (file);
        updateSampleLabels();
    };
    importLibrarySampleButton.onClick = [this] { openSampleChooser(); };
    deleteLibrarySampleButton.onClick = [this]
    {
        const auto file = getSelectedLibraryFile();
        if (file.existsAsFile())
        {
            audioProcessor.deleteUserSample (file);
            refreshLibraryList();
            updateSampleLabels();
        }
    };
    clearLoadedSampleButton.onClick = [this]
    {
        audioProcessor.clearSample();
        updateSampleLabels();
    };
    addAndMakeVisible (refreshLibraryButton);
    addAndMakeVisible (loadLibrarySampleButton);
    addAndMakeVisible (importLibrarySampleButton);
    addAndMakeVisible (deleteLibrarySampleButton);
    addAndMakeVisible (clearLoadedSampleButton);
    configureLabel (libraryStatusLabel, "");

    showTab (false);
    refreshLibraryList();
    updateSampleLabels();
}

void HybridSynthAudioProcessorEditor::configureLabel (juce::Label& label,
                                                       const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, mutedTextColour);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void HybridSynthAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                        juce::Label& label,
                                                        const juce::String& labelText)
{
    configureLabel (label, labelText);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::thumbColourId, textColour);
    slider.setColour (juce::Slider::textBoxTextColourId, textColour);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panelColour);
    slider.setColour (juce::Slider::textBoxOutlineColourId, panelEdgeColour);
    addAndMakeVisible (slider);
}

void HybridSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);
    g.setColour (panelEdgeColour);
    g.drawLine (18.0f, 58.0f, (float) getWidth() - 18.0f, 58.0f, 1.0f);

    auto content = getLocalBounds().reduced (18);
    content.removeFromTop (100);
    g.setColour (panelColour);
    g.fillRoundedRectangle (content.toFloat(), 12.0f);
    g.setColour (panelEdgeColour);
    g.drawRoundedRectangle (content.toFloat(), 12.0f, 1.0f);
}

void HybridSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    auto header = area.removeFromTop (48);
    titleLabel.setBounds (header.removeFromLeft (260));
    settingsButton.setBounds (header.removeFromRight (42).reduced (3));

    auto tabs = area.removeFromTop (42);
    synthTab.setBounds (tabs.removeFromLeft (110).reduced (2, 3));
    userTab.setBounds (tabs.removeFromLeft (110).reduced (2, 3));

    auto content = area.reduced (10, 8);
    if (showingUser)
    {
        userTitle.setBounds (content.removeFromTop (42));
        userDescription.setBounds (content.removeFromTop (32));
        auto controls = content.removeFromTop (42);
        librarySamplesBox.setBounds (controls.removeFromLeft (std::min (420, controls.getWidth() - 330)).reduced (2));
        refreshLibraryButton.setBounds (controls.removeFromLeft (88).reduced (2));
        loadLibrarySampleButton.setBounds (controls.removeFromLeft (130).reduced (2));
        importLibrarySampleButton.setBounds (controls.removeFromLeft (110).reduced (2));

        auto actions = content.removeFromTop (42);
        deleteLibrarySampleButton.setBounds (actions.removeFromLeft (150).reduced (2));
        clearLoadedSampleButton.setBounds (actions.removeFromLeft (180).reduced (2));
        libraryStatusLabel.setBounds (content.removeFromTop (34));
        return;
    }

    const auto topHeight = juce::jmax (360, content.getHeight() - 130);
    auto top = content.removeFromTop (topHeight);
    auto source = top.removeFromLeft ((int) (top.getWidth() * 0.46f)).reduced (10);
    auto envelope = top.reduced (10);

    sourceTitle.setBounds (source.removeFromTop (24));
    auto waveRow = source.removeFromTop (30);
    waveformBox.setBounds (waveRow.removeFromLeft (150).reduced (2));
    waveformDisplay.setBounds (source.removeFromTop (75).reduced (2, 3));

    auto waveControls = source.removeFromTop (70);
    auto morphCell = waveControls.removeFromLeft (waveControls.getWidth() / 2);
    morphLabel.setBounds (morphCell.removeFromTop (22));
    morphSlider.setBounds (morphCell.reduced (3, 0));
    auto levelCell = waveControls;
    wavetableLevelLabel.setBounds (levelCell.removeFromTop (22));
    wavetableLevelSlider.setBounds (levelCell.reduced (3, 0));

    sampleTitle.setBounds (source.removeFromTop (22));
    auto importRow = source.removeFromTop (26);
    importSampleButton.setBounds (importRow.removeFromLeft (120).reduced (2));
    sampleLoopButton.setBounds (importRow.removeFromLeft (70).reduced (2));
    sampleNameLabel.setBounds (importRow);
    sampleWaveformDisplay.setBounds (source.removeFromTop (55).reduced (2, 2));

    auto sampleControls = source;
    const auto sampleWidth = sampleControls.getWidth() / 4;
    auto sampleCell = sampleControls.removeFromLeft (sampleWidth);
    sampleLevelLabel.setBounds (sampleCell.removeFromTop (22));
    sampleLevelSlider.setBounds (sampleCell.reduced (1, 0));
    sampleCell = sampleControls.removeFromLeft (sampleWidth);
    sampleStartLabel.setBounds (sampleCell.removeFromTop (22));
    sampleStartSlider.setBounds (sampleCell.reduced (1, 0));
    sampleCell = sampleControls.removeFromLeft (sampleWidth);
    sampleEndLabel.setBounds (sampleCell.removeFromTop (22));
    sampleEndSlider.setBounds (sampleCell.reduced (1, 0));
    sampleCell = sampleControls;
    sampleRootLabel.setBounds (sampleCell.removeFromTop (22));
    sampleRootSlider.setBounds (sampleCell.reduced (1, 0));

    envelopeTitle.setBounds (envelope.removeFromTop (24));
    envelopeDisplay.setBounds (envelope.removeFromTop (190).reduced (2, 4));

    auto envelopeControls = envelope.removeFromTop (115);
    const auto envWidth = envelopeControls.getWidth() / 4;
    auto envCell = envelopeControls.removeFromLeft (envWidth);
    attackLabel.setBounds (envCell.removeFromTop (22));
    attackSlider.setBounds (envCell.reduced (1, 0));
    envCell = envelopeControls.removeFromLeft (envWidth);
    decayLabel.setBounds (envCell.removeFromTop (22));
    decaySlider.setBounds (envCell.reduced (1, 0));
    envCell = envelopeControls.removeFromLeft (envWidth);
    sustainLabel.setBounds (envCell.removeFromTop (22));
    sustainSlider.setBounds (envCell.reduced (1, 0));
    envCell = envelopeControls;
    releaseLabel.setBounds (envCell.removeFromTop (22));
    releaseSlider.setBounds (envCell.reduced (1, 0));

    auto footer = content.reduced (10, 4);
    auto voiceArea = footer.removeFromLeft (250);
    voiceModeLabel.setBounds (voiceArea.removeFromTop (24));
    voiceModeBox.setBounds (voiceArea.removeFromTop (36).reduced (2));

    auto gainArea = footer.removeFromLeft (150);
    masterGainLabel.setBounds (gainArea.removeFromTop (24));
    masterGainSlider.setBounds (gainArea.reduced (2, 0));

    auto randomArea = footer;
    auto randomTop = randomArea.removeFromTop (38);
    randomizeEnvelopeButton.setBounds (randomTop.removeFromLeft (130).reduced (2));
    randomizeWavetableButton.setBounds (randomTop.removeFromLeft (140).reduced (2));
    randomizeSampleButton.setBounds (randomTop.removeFromLeft (135).reduced (2));
    randomizeAllButton.setBounds (randomArea.removeFromTop (38).removeFromLeft (150).reduced (2));
}

void HybridSynthAudioProcessorEditor::showTab (bool user)
{
    showingUser = user;
    const auto showSynth = ! user;

    sourceTitle.setVisible (showSynth);
    waveformBox.setVisible (showSynth);
    waveformDisplay.setVisible (showSynth);
    morphLabel.setVisible (showSynth);
    morphSlider.setVisible (showSynth);
    wavetableLevelLabel.setVisible (showSynth);
    wavetableLevelSlider.setVisible (showSynth);
    sampleTitle.setVisible (showSynth);
    importSampleButton.setVisible (showSynth);
    sampleNameLabel.setVisible (showSynth);
    sampleWaveformDisplay.setVisible (showSynth);
    sampleLevelLabel.setVisible (showSynth);
    sampleLevelSlider.setVisible (showSynth);
    sampleStartLabel.setVisible (showSynth);
    sampleStartSlider.setVisible (showSynth);
    sampleEndLabel.setVisible (showSynth);
    sampleEndSlider.setVisible (showSynth);
    sampleRootLabel.setVisible (showSynth);
    sampleRootSlider.setVisible (showSynth);
    sampleLoopButton.setVisible (showSynth);
    envelopeTitle.setVisible (showSynth);
    envelopeDisplay.setVisible (showSynth);
    attackLabel.setVisible (showSynth);
    attackSlider.setVisible (showSynth);
    decayLabel.setVisible (showSynth);
    decaySlider.setVisible (showSynth);
    sustainLabel.setVisible (showSynth);
    sustainSlider.setVisible (showSynth);
    releaseLabel.setVisible (showSynth);
    releaseSlider.setVisible (showSynth);
    voiceModeLabel.setVisible (showSynth);
    voiceModeBox.setVisible (showSynth);
    masterGainLabel.setVisible (showSynth);
    masterGainSlider.setVisible (showSynth);
    randomizeEnvelopeButton.setVisible (showSynth);
    randomizeWavetableButton.setVisible (showSynth);
    randomizeSampleButton.setVisible (showSynth);
    randomizeAllButton.setVisible (showSynth);

    userTitle.setVisible (user);
    userDescription.setVisible (user);
    librarySamplesBox.setVisible (user);
    refreshLibraryButton.setVisible (user);
    loadLibrarySampleButton.setVisible (user);
    importLibrarySampleButton.setVisible (user);
    deleteLibrarySampleButton.setVisible (user);
    clearLoadedSampleButton.setVisible (user);
    libraryStatusLabel.setVisible (user);

    synthTab.setColour (juce::TextButton::buttonColourId,
                        showSynth ? accentColour.withAlpha (0.24f) : panelColour);
    userTab.setColour (juce::TextButton::buttonColourId,
                       user ? accentColour.withAlpha (0.24f) : panelColour);

    if (user)
        refreshLibraryList();
    resized();
    repaint();
}

void HybridSynthAudioProcessorEditor::openSampleChooser()
{
    sampleChooser = std::make_unique<juce::FileChooser> (
        "Import an audio sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");

    juce::Component::SafePointer<HybridSynthAudioProcessorEditor> safeThis (this);
    sampleChooser->launchAsync (juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
        [safeThis] (const juce::FileChooser& chooser)
        {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                if (safeThis->audioProcessor.importSampleToLibrary (file))
                {
                    safeThis->refreshLibraryList();
                    safeThis->updateSampleLabels();
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::MessageBoxIconType::WarningIcon, "Sample import failed",
                        "The file could not be decoded or copied into the SQUIGGLE! sample library.");
                }
            }
        });
}

void HybridSynthAudioProcessorEditor::refreshLibraryList()
{
    const auto files = audioProcessor.getUserSampleFiles();
    librarySamplesBox.clear (juce::dontSendNotification);
    for (int i = 0; i < files.size(); ++i)
        librarySamplesBox.addItem (files.getReference (i).getFileName(), i + 1);

    if (! files.isEmpty())
        librarySamplesBox.setSelectedId (1, juce::dontSendNotification);

    libraryStatusLabel.setText (juce::String (files.size()) + " sample(s) in the local library",
                                juce::dontSendNotification);
}

juce::File HybridSynthAudioProcessorEditor::getSelectedLibraryFile() const
{
    const auto selectedId = librarySamplesBox.getSelectedId();
    const auto files = audioProcessor.getUserSampleFiles();
    const auto index = selectedId - 1;
    return juce::isPositiveAndBelow (index, files.size())
        ? files.getReference (index) : juce::File();
}

void HybridSynthAudioProcessorEditor::updateSampleLabels()
{
    const auto name = audioProcessor.getLoadedSampleName();
    sampleNameLabel.setText (name, juce::dontSendNotification);
    sampleWaveformDisplay.repaint();
    libraryStatusLabel.setText ("Loaded: " + name, juce::dontSendNotification);
}

void HybridSynthAudioProcessorEditor::showTechnicalSettingsMessage()
{
    juce::AlertWindow::showMessageBoxAsync (
        juce::MessageBoxIconType::InfoIcon,
        "Technical Settings",
        "Processing Quality modes and interface-size preferences are planned for a later development stage.\n\n"
        "The current build keeps DSP settings fixed to avoid exposing controls that are not implemented yet.");
}
