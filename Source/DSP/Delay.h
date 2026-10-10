#pragma once

#include <juce_dsp/juce_dsp.h>

// Delay digital com repeticoes que vao escurecendo (filtro no feedback),
// como um delay analogico/tape simples.
class Delay
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setTimeMs (float ms) noexcept    { timeMs = ms; }
    void setFeedback (float fb) noexcept  { feedback = fb; }
    void setTone (float t01) noexcept     { tone01 = t01; }
    void setMix (float m) noexcept        { mix = m; }

    void process (float* data, int numSamples) noexcept;

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> line { 1 };
    juce::SmoothedValue<float> delaySamples, mixSmoothed;
    double sr = 48000.0;
    float timeMs = 380.0f, feedback = 0.35f, tone01 = 0.6f, mix = 0.3f;
    float lpState = 0.0f;
};
