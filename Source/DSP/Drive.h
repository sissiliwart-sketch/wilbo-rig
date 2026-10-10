#pragma once

#include <juce_dsp/juce_dsp.h>

// Pedal de saturacao com 3 modos:
//  0 Overdrive  - corte de graves antes do clip (estilo "tube screamer"), clip suave
//  1 Distortion - mais ganho, clip mais duro
//  2 Fuzz       - clip assimetrico e agressivo
// O clip roda com oversampling 4x para nao gerar aliasing (o "chiado metalico").
class Drive
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setType (int t) noexcept       { type = t; }
    void setGain (float g01) noexcept   { gain01 = g01; }
    void setTone (float t01) noexcept   { tone01 = t01; }
    void setLevelDb (float db) noexcept { levelDb = db; }

    void process (float* data, int numSamples) noexcept;

    static float clip (float x, int type) noexcept;

private:
    void updateFilters();

    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::IIR::Filter<float> preHighPass, preMidBump, postLowPass;
    juce::SmoothedValue<float> driveSmoothed, levelSmoothed;

    double sr = 48000.0;
    int type = 0, lastType = -1;
    float gain01 = 0.5f, tone01 = 0.5f, levelDb = 0.0f, lastTone = -1.0f;
};
