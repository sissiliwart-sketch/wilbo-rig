#include "Wah.h"
#include <cmath>

void Wah::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });

    envAttack  = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));  // 4 ms
    envRelease = 1.0f - std::exp (-1.0f / (0.120f * (float) sr));  // 120 ms
    posCoeff   = 1.0f - std::exp (-1.0f / (0.015f * (float) sr));  // 15 ms (evita "zipper")
    reset();
}

void Wah::reset()
{
    filter.reset();
    envelope = 0.0f;
    smoothedPos = pedal;
    counter = 0;
}

void Wah::process (float* data, int numSamples) noexcept
{
    constexpr float fMin = 350.0f, fMax = 2200.0f;
    filter.setResonance (resonance);

    for (int i = 0; i < numSamples; ++i)
    {
        const float x = data[i];

        float target = pedal;
        if (autoMode)
        {
            const float level = std::abs (x);
            envelope += (level > envelope ? envAttack : envRelease) * (level - envelope);
            const float gain = 2.0f + 40.0f * sensitivity;
            target = juce::jlimit (0.0f, 1.0f, envelope * gain);
        }

        smoothedPos += posCoeff * (target - smoothedPos);

        if ((counter++ & 15) == 0) // recalcula o filtro a cada 16 amostras
            filter.setCutoffFrequency (fMin * std::pow (fMax / fMin, smoothedPos));

        const float bp = filter.processSample (0, x);
        // um pouco do sinal seco mantem o corpo do som
        data[i] = 0.12f * x + bp * (1.5f + 0.15f * resonance);
    }
}
