#include "PluginProcessor.h"
#include "PluginEditor.h"

SlapboxAudioProcessor::SlapboxAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    timeParam     = apvts.getRawParameterValue ("time");
    feedbackParam = apvts.getRawParameterValue ("feedback");
    mixParam      = apvts.getRawParameterValue ("mix");
    widthParam    = apvts.getRawParameterValue ("width");
    hpfParam      = apvts.getRawParameterValue ("hpf");
    lpfParam      = apvts.getRawParameterValue ("lpf");
    bypassParam   = apvts.getRawParameterValue ("bypass");
    driveParam    = apvts.getRawParameterValue ("drive");
}

juce::AudioProcessorValueTreeState::ParameterLayout SlapboxAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    auto suffix = [] (const char* unit)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [unit] (float v, int) { return juce::String (juce::roundToInt (v)) + " " + unit; });
    };

    // Both filters share one 20 Hz - 20 kHz range, because they live on one two-thumb slider.
    juce::NormalisableRange<float> freq (20.0f, 20000.0f, 1.0f);
    freq.setSkewForCentre (2000.0f);   // middle of the slider = 2 kHz; ~25% = 220 Hz, ~75% = 7.7 kHz
    const auto hz = juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
    {
        return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz"
                           : juce::String (v / 1000.0f, 1) + " kHz";
    });

    return {
        std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false),
        std::make_unique<P> (juce::ParameterID { "hpf", 1 }, "Low Cut",  freq, 80.0f,    hz),
        std::make_unique<P> (juce::ParameterID { "lpf", 1 }, "High Cut", freq, 12000.0f, hz),
        std::make_unique<P> (juce::ParameterID { "time", 1 }, "Time",
                             juce::NormalisableRange<float> (40.0f, maxDelayMs, 1.0f), 80.0f, suffix ("ms")),
        std::make_unique<P> (juce::ParameterID { "feedback", 1 }, "Feedback",
                             juce::NormalisableRange<float> (0.0f, 60.0f, 1.0f), 10.0f, suffix ("%")),
        std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",
                             juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 35.0f, suffix ("%")),
        std::make_unique<P> (juce::ParameterID { "drive", 1 }, "Saturation",
                             juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f, suffix ("%")),
        std::make_unique<P> (juce::ParameterID { "width", 1 }, "Width",
                             juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, suffix ("%"))
    };
}

bool SlapboxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && out == layouts.getMainInputChannelSet();
}

void SlapboxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplesPerMs = (float) sampleRate * 0.001f;
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                        (juce::uint32) getTotalNumOutputChannels() };

    delayLine.setMaximumDelayInSamples ((int) std::ceil ((maxDelayMs + maxSpreadMs) * samplesPerMs) + 2);
    delayLine.prepare (spec);
    delayLine.reset();

    hpf.prepare (spec);
    hpf.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    hpf.reset();
    lpf.prepare (spec);
    lpf.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lpf.reset();

    timeMs.reset (sampleRate, 0.05);
    feedback.reset (sampleRate, 0.02);
    mix.reset (sampleRate, 0.02);
    width.reset (sampleRate, 0.05);
    active.reset (sampleRate, 0.02);
    drive.reset (sampleRate, 0.05);
    drive.setCurrentAndTargetValue (driveParam->load() * 0.01f);
    active.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    timeMs.setCurrentAndTargetValue (timeParam->load());
    feedback.setCurrentAndTargetValue (feedbackParam->load() * 0.01f);
    mix.setCurrentAndTargetValue (mixParam->load() * 0.01f);
    width.setCurrentAndTargetValue (widthParam->load() * 0.01f);
}

void SlapboxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = getTotalNumOutputChannels();
    const int numSamples  = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < numChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    timeMs.setTargetValue (timeParam->load());
    feedback.setTargetValue (feedbackParam->load() * 0.01f);
    mix.setTargetValue (mixParam->load() * 0.01f);
    width.setTargetValue (widthParam->load() * 0.01f);
    active.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    drive.setTargetValue (driveParam->load() * 0.01f);
    hpf.setCutoffFrequency (hpfParam->load());
    lpf.setCutoffFrequency (juce::jmin (lpfParam->load(), samplesPerMs * 1000.0f * 0.45f));   // stay below Nyquist

    for (int i = 0; i < numSamples; ++i)
    {
        const float d  = timeMs.getNextValue() * samplesPerMs;
        const float fb = feedback.getNextValue();
        const float m  = mix.getNextValue();
        const float w  = width.getNextValue();
        const float a  = active.getNextValue();
        const float k  = 1.0f + 5.0f * drive.getNextValue();   // 0% = nearly clean, 100% = heavy
        const float makeup = std::sqrt (k);   // crossfade to dry when bypassed, so it doesn't click

        if (numChannels == 2)
        {
            float* L = buffer.getWritePointer (0);
            float* R = buffer.getWritePointer (1);
            const float inL = L[i], inR = R[i];

            // Width: left echo comes a little early, right a little late (works on mono sources too).
            const float spread = w * maxSpreadMs * samplesPerMs;
            const float wetL = lpf.processSample (0, hpf.processSample (0, delayLine.popSample (0, d - spread)));
            const float wetR = lpf.processSample (1, hpf.processSample (1, delayLine.popSample (1, d + spread)));
            delayLine.pushSample (0, saturate (inL + wetL * fb, k));
            delayLine.pushSample (1, saturate (inR + wetR * fb, k));

            // ...and at 0% the echo is summed to mono so it sits dead centre.
            const float mid = 0.5f * (wetL + wetR) * makeup;
            const float outL = inL * (1.0f - m) + (mid + (wetL * makeup - mid) * w) * m;
            const float outR = inR * (1.0f - m) + (mid + (wetR * makeup - mid) * w) * m;
            L[i] = inL + (outL - inL) * a;
            R[i] = inR + (outR - inR) * a;
        }
        else if (numChannels == 1)
        {
            float* data = buffer.getWritePointer (0);
            const float dry = data[i];
            const float wet = lpf.processSample (0, hpf.processSample (0, delayLine.popSample (0, d)));
            delayLine.pushSample (0, saturate (dry + wet * fb, k));
            data[i] = dry + (dry * (1.0f - m) + wet * makeup * m - dry) * a;
        }
    }
}

void SlapboxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SlapboxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SlapboxAudioProcessor::createEditor()
{
    return new SlapboxAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlapboxAudioProcessor();
}
