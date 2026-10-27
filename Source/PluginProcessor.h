#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SlapboxAudioProcessor : public juce::AudioProcessor
{
public:
    SlapboxAudioProcessor();
    ~SlapboxAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Lets the host's own bypass button and ours stay in sync.
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr float maxDelayMs  = 120.0f;
    static constexpr float maxSpreadMs = 12.0f;   // Width offsets L/R echo times by up to this much

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 1 };

    // Smoothed so knob moves don't click or zipper.
    juce::SmoothedValue<float> timeMs, feedback, mix, width, drive, active;   // active: 1 = on, 0 = bypassed

    std::atomic<float>* timeParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* widthParam    = nullptr;
    std::atomic<float>* hpfParam      = nullptr;
    std::atomic<float>* lpfParam      = nullptr;
    std::atomic<float>* bypassParam   = nullptr;
    std::atomic<float>* driveParam    = nullptr;

    // Filters sit inside the feedback loop, so each repeat gets thinner/darker.
    juce::dsp::StateVariableTPTFilter<float> hpf, lpf;

    // Soft tanh saturation on everything entering the delay, so each repeat gets warmer.
    // Gain never exceeds 1 here, so feedback can't run away; level is made up on the output instead.
    static float saturate (float x, float k) { return std::tanh (k * x) / k; }

    float samplesPerMs = 44.1f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapboxAudioProcessor)
};
