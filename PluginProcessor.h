#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class WorshipVoiceFXProcessor : public juce::AudioProcessor
{
public:
    WorshipVoiceFXProcessor();
    ~WorshipVoiceFXProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Worship Voice FX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    using Coeff = juce::dsp::IIR::Coefficients<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, Coeff>;
    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    void updateParameters();
    void processDeEsser (juce::AudioBuffer<float>&);
    void processSaturation (juce::AudioBuffer<float>&);
    void processDelay (juce::AudioBuffer<float>&);
    void processReverb (juce::AudioBuffer<float>&);

    struct Ptrs
    {
        std::atomic<float> *inGain, *lowCut, *deEss, *sat, *compThresh, *compRatio, *compMakeup,
                           *warmth, *presence, *air, *chorus,
                           *revSize, *revDamp, *revPre, *revTone, *revMix,
                           *dlySync, *dlyTime, *dlyFb, *dlyTone, *dlyMix,
                           *mix, *outGain;
    } p {};

    struct Last { float lowCut = -1e9f, warm = -1e9f, pres = -1e9f, air = -1e9f, revTone = -1e9f; } last;
    struct Cur  { float deEss = 0, sat = 0, dlyFb = 0, dlyTone = 4000; } cur;

    double sr = 44100.0;

    juce::dsp::Gain<float> inGain, makeup, outGain;
    Duplicator hpf, eqLow, eqMid, eqHigh, revHP, revLP;
    juce::dsp::Compressor<float> comp;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    Delay echo, preDelay;
    juce::dsp::DryWetMixer<float> mixer;

    juce::dsp::IIR::Filter<float> deHP[2];
    float deEnv = 0.0f, deAtk = 0.0f, deRel = 0.0f;
    float dlyLP[2] { 0.0f, 0.0f };

    juce::AudioBuffer<float> deBand, revBuf;
    juce::SmoothedValue<float> smDelay, smDlyMix, smRevMix;

    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WorshipVoiceFXProcessor)
};
