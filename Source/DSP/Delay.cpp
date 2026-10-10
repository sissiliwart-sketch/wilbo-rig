#include "Delay.h"
#include <cmath>

void Delay::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    line.setMaximumDelayInSamples ((int) (sampleRate * 2.1));
    line.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });

    delaySamples.reset (sampleRate, 0.25); // troca de tempo suave (efeito "tape")
    delaySamples.setCurrentAndTargetValue (timeMs * 0.001f * (float) sr);
    mixSmoothed.reset (sampleRate, 0.03);
    mixSmoothed.setCurrentAndTargetValue (mix);
    reset();
}

void Delay::reset()
{
    line.reset();
    lpState = 0.0f;
}

void Delay::process (float* data, int numSamples) noexcept
{
    delaySamples.setTargetValue (juce::jlimit (1.0f, (float) sr * 2.05f, timeMs * 0.001f * (float) sr));
    mixSmoothed.setTargetValue (mix);

    const float cutoff = 1200.0f * std::pow (12000.0f / 1200.0f, tone01);
    const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmin (cutoff, (float) sr * 0.45f) / (float) sr);

    for (int i = 0; i < numSamples; ++i)
    {
        const float x = data[i];
        const float delayed = line.popSample (0, delaySamples.getNextValue());

        lpState += a * (delayed - lpState);
        // tanh no feedback impede que o som "exploda" com feedback alto
        line.pushSample (0, x + std::tanh (lpState * feedback));

        const float m = mixSmoothed.getNextValue();
        data[i] = x * (1.0f - 0.5f * m) + delayed * m;
    }
}
