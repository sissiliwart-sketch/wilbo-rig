#include "Drive.h"
#include <cmath>

using Coeffs = juce::dsp::IIR::Coefficients<float>;

void Drive::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    oversampling.initProcessing ((size_t) maxBlockSize);

    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 1 };
    preHighPass.prepare (spec);
    preMidBump.prepare (spec);
    postLowPass.prepare (spec);

    driveSmoothed.reset (sampleRate, 0.03);
    levelSmoothed.reset (sampleRate, 0.03);
    lastType = -1;
    lastTone = -1.0f;
    reset();
}

void Drive::reset()
{
    oversampling.reset();
    preHighPass.reset();
    preMidBump.reset();
    postLowPass.reset();
}

void Drive::updateFilters()
{
    if (type != lastType)
    {
        // Overdrive corta mais grave (som mais "medio"); fuzz deixa passar quase tudo
        const float hp  = type == 0 ? 600.0f : (type == 1 ? 180.0f : 90.0f);
        const float bump = type == 0 ? 3.0f : (type == 1 ? 1.5f : 0.0f);
        *preHighPass.coefficients = *Coeffs::makeFirstOrderHighPass (sr, hp);
        *preMidBump.coefficients  = *Coeffs::makePeakFilter (sr, 900.0f, 0.8f,
                                                             juce::Decibels::decibelsToGain (bump));
        lastType = type;
    }

    if (std::abs (tone01 - lastTone) > 1.0e-4f)
    {
        const float cutoff = 900.0f * std::pow (9000.0f / 900.0f, tone01); // 900 Hz .. 9 kHz
        *postLowPass.coefficients = *Coeffs::makeLowPass (sr, juce::jmin (cutoff, (float) sr * 0.45f), 0.707f);
        lastTone = tone01;
    }
}

float Drive::clip (float x, int t) noexcept
{
    switch (t)
    {
        case 0:  // soft clip levemente assimetrico (diodos)
            return std::tanh (x + 0.1f) - std::tanh (0.1f);

        case 1:  // clip mais duro
        {
            const float a = std::abs (x);
            return x / std::pow (1.0f + std::pow (a, 3.0f), 1.0f / 3.0f);
        }

        default: // fuzz: assimetrico, corta forte o lado negativo
        {
            const float y = x > 0.0f ? std::tanh (x) : std::tanh (x * 2.2f) * 0.6f;
            return juce::jlimit (-0.8f, 0.9f, y * 1.2f);
        }
    }
}

void Drive::process (float* data, int numSamples) noexcept
{
    updateFilters();

    // Ganho de entrada em dB segundo o tipo
    const float maxDb = type == 0 ? 36.0f : (type == 1 ? 48.0f : 54.0f);
    const float minDb = type == 0 ? 0.0f  : 10.0f;
    driveSmoothed.setTargetValue (juce::Decibels::decibelsToGain (minDb + gain01 * (maxDb - minDb)));

    // compensa o volume para que o knob "Level" fique perto de unidade
    const float makeup = type == 2 ? -6.0f : -3.0f;
    levelSmoothed.setTargetValue (juce::Decibels::decibelsToGain (levelDb + makeup));

    for (int i = 0; i < numSamples; ++i)
        data[i] = preMidBump.processSample (preHighPass.processSample (data[i])) * driveSmoothed.getNextValue();

    float* channels[] = { data };
    juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    float* u = up.getChannelPointer (0);

    for (size_t i = 0; i < up.getNumSamples(); ++i)
        u[i] = clip (u[i], type);

    oversampling.processSamplesDown (block);

    for (int i = 0; i < numSamples; ++i)
        data[i] = postLowPass.processSample (data[i]) * levelSmoothed.getNextValue();
}
