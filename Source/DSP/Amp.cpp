#include "Amp.h"
#include <cmath>

using Coeffs = juce::dsp::IIR::Coefficients<float>;

void Amp::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    oversampling.initProcessing ((size_t) maxBlockSize);
    osRate = sampleRate * (double) oversampling.getOversamplingFactor();

    const juce::dsp::ProcessSpec spec   { sr,     (juce::uint32) maxBlockSize, 1 };
    const juce::dsp::ProcessSpec osSpec { osRate, (juce::uint32) maxBlockSize * 4, 1 };

    inputHighPass.prepare (spec);
    interStageHighPass.prepare (osSpec);
    interStageLowPass.prepare (osSpec);
    for (auto* f : { &bassShelf, &midPeak, &trebleShelf, &presenceShelf })
        f->prepare (spec);

    *inputHighPass.coefficients      = *Coeffs::makeHighPass (sr, 85.0f, 0.707f);
    *interStageHighPass.coefficients = *Coeffs::makeFirstOrderHighPass (osRate, 25.0f);  // capacitor de acoplamento
    *interStageLowPass.coefficients  = *Coeffs::makeFirstOrderLowPass (osRate, 9000.0f);  // capacitancia Miller

    gainSmoothed.reset (sr, 0.03);
    masterSmoothed.reset (sr, 0.03);
    lastTone = { -1, -1, -1, -1, -1, 0 };
    reset();
}

void Amp::reset()
{
    oversampling.reset();
    for (auto* f : { &inputHighPass, &interStageHighPass, &interStageLowPass,
                     &bassShelf, &midPeak, &trebleShelf, &presenceShelf })
        f->reset();
}

void Amp::updateTone()
{
    auto changed = [] (float a, float b) { return std::abs (a - b) > 1.0e-4f; };
    auto toDb = [] (float knob, float range) { return (knob - 5.0f) / 5.0f * range; };
    auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    if (changed (settings.bass, lastTone.bass))
        *bassShelf.coefficients = *Coeffs::makeLowShelf (sr, 120.0f, 0.7f, g (toDb (settings.bass, 12.0f)));

    if (changed (settings.mid, lastTone.mid))
        *midPeak.coefficients = *Coeffs::makePeakFilter (sr, 750.0f, 0.8f, g (toDb (settings.mid, 10.0f)));

    if (changed (settings.treble, lastTone.treble))
        *trebleShelf.coefficients = *Coeffs::makeHighShelf (sr, 2800.0f, 0.7f, g (toDb (settings.treble, 12.0f)));

    if (changed (settings.presence, lastTone.presence))
        *presenceShelf.coefficients = *Coeffs::makeHighShelf (sr, juce::jmin (5500.0f, (float) sr * 0.4f), 0.7f,
                                                              g (toDb (settings.presence, 8.0f)));
    lastTone = settings;
}

// Estagio de valvula: satura mais de um lado que do outro (gera harmonicos pares)
float Amp::tubeStage (float x, float bias) noexcept
{
    return std::tanh (x + bias) - std::tanh (bias);
}

void Amp::process (float* data, int numSamples) noexcept
{
    updateTone();

    // Gain 0..10 -> 0..+45 dB
    gainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (settings.gain * 4.5f));
    masterSmoothed.setTargetValue (juce::Decibels::decibelsToGain (settings.masterDb));

    for (int i = 0; i < numSamples; ++i)
        data[i] = inputHighPass.processSample (data[i]) * gainSmoothed.getNextValue();

    float* channels[] = { data };
    juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    float* u = up.getChannelPointer (0);

    for (size_t i = 0; i < up.getNumSamples(); ++i)
    {
        float s = tubeStage (u[i] * 1.5f, 0.2f);
        s = interStageLowPass.processSample (interStageHighPass.processSample (s));
        s = tubeStage (s * 2.5f, -0.15f);
        u[i] = s * 0.3f;  // trim: deixa espaco (headroom) para EQ, gabinete e efeitos
    }

    oversampling.processSamplesDown (block);

    for (int i = 0; i < numSamples; ++i)
    {
        float s = data[i];
        s = bassShelf.processSample (s);
        s = midPeak.processSample (s);
        s = trebleShelf.processSample (s);
        s = presenceShelf.processSample (s);
        data[i] = s * masterSmoothed.getNextValue();
    }
}
