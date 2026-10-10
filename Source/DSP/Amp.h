#pragma once

#include <juce_dsp/juce_dsp.h>

// Simulador de amplificador valvulado (versao simplificada):
//   entrada -> 2 estagios de pre-amp (saturacao assimetrica, oversampling 4x)
//           -> tonestack (Bass / Mid / Treble) -> Presence -> Master
class Amp
{
public:
    struct Settings
    {
        float gain = 4.0f, bass = 5.0f, mid = 5.0f, treble = 5.0f, presence = 5.0f; // 0..10
        float masterDb = -6.0f;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void setSettings (const Settings& s) noexcept { settings = s; }
    void process (float* data, int numSamples) noexcept;

private:
    void updateTone();
    static float tubeStage (float x, float bias) noexcept;

    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    juce::dsp::IIR::Filter<float> inputHighPass, interStageHighPass, interStageLowPass;
    juce::dsp::IIR::Filter<float> bassShelf, midPeak, trebleShelf, presenceShelf;
    juce::SmoothedValue<float> gainSmoothed, masterSmoothed;

    Settings settings, lastTone { -1, -1, -1, -1, -1, 0 };
    double sr = 48000.0, osRate = 192000.0;
};
