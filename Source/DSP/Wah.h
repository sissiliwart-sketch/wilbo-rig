#pragma once

#include <juce_dsp/juce_dsp.h>

// Wah-wah: filtro passa-banda ressonante que varre de ~350 Hz a ~2.2 kHz.
// Modo manual: a posicao vem do knob "Pedal" (ou de um pedal MIDI no futuro).
// Modo auto: a posicao segue a forca da palhetada (envelope follower).
class Wah
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setPedal (float position01) noexcept { pedal = position01; }
    void setQ (float q) noexcept              { resonance = q; }
    void setAuto (bool shouldBeAuto) noexcept { autoMode = shouldBeAuto; }
    void setSensitivity (float s01) noexcept  { sensitivity = s01; }

    void process (float* data, int numSamples) noexcept;

    float getCurrentPosition() const noexcept { return smoothedPos; } // p/ desenhar na tela

private:
    juce::dsp::StateVariableTPTFilter<float> filter;
    double sr = 48000.0;

    float pedal = 0.5f, resonance = 5.0f, sensitivity = 0.5f;
    bool  autoMode = false;

    float envelope = 0.0f, smoothedPos = 0.5f;
    float envAttack = 0.0f, envRelease = 0.0f, posCoeff = 0.0f;
    int   counter = 0;
};
