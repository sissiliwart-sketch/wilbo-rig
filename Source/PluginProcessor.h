#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/Tuner.h"
#include "DSP/Wah.h"
#include "DSP/Drive.h"
#include "DSP/Amp.h"
#include "DSP/Cabinet.h"
#include "DSP/Delay.h"

// O "cerebro" da pedaleira: recebe o audio da guitarra e passa pela cadeia
//
//  Entrada -> Gate -> [Afinador] -> Wah -> Drive -> Amp -> Gabinete -> Delay -> Reverb -> Saida
//
class WilboRigProcessor : public juce::AudioProcessor
{
public:
    WilboRigProcessor();
    ~WilboRigProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Wilbo Rig"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Acesso para a interface
    juce::AudioProcessorValueTreeState apvts;
    Tuner& getTuner() noexcept { return tuner; }
    Cabinet& getCabinet() noexcept { return cabinet; }
    float getInputLevel() const noexcept  { return inputLevel.load(); }
    float getOutputLevel() const noexcept { return outputLevel.load(); }
    float getWahPosition() const noexcept { return wahPosition.load(); }

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool  on (const char* id) const { return p (id) > 0.5f; }

    juce::dsp::NoiseGate<float> gate;
    Tuner tuner;
    Wah wah;
    Drive drive;
    Amp amp;
    Cabinet cabinet;
    Delay delay;
    juce::dsp::Reverb reverb;

    juce::SmoothedValue<float> inGainSmoothed, outGainSmoothed;
    juce::AudioBuffer<float> monoBuffer;

    std::atomic<float> inputLevel { 0.0f }, outputLevel { 0.0f }, wahPosition { 0.5f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WilboRigProcessor)
};
