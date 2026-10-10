#include "Cabinet.h"

using Coeffs = juce::dsp::IIR::Coefficients<float>;

void Cabinet::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 1 };

    convolution.prepare (spec);
    for (auto* f : { &hp, &lp1, &lp2, &peak, &notch })
        f->prepare (spec);

    const float top = juce::jmin (5000.0f, (float) sr * 0.4f);
    *hp.coefficients    = *Coeffs::makeHighPass (sr, 75.0f, 0.8f);
    *lp1.coefficients   = *Coeffs::makeLowPass (sr, top, 0.6f);
    *lp2.coefficients   = *Coeffs::makeLowPass (sr, top, 0.9f);
    *peak.coefficients  = *Coeffs::makePeakFilter (sr, 2300.0f, 1.2f, juce::Decibels::decibelsToGain (4.0f));
    *notch.coefficients = *Coeffs::makePeakFilter (sr, 420.0f, 1.0f, juce::Decibels::decibelsToGain (-3.0f));
    reset();
}

void Cabinet::reset()
{
    convolution.reset();
    for (auto* f : { &hp, &lp1, &lp2, &peak, &notch })
        f->reset();
}

bool Cabinet::loadImpulseResponse (const juce::File& wavFile)
{
    if (! wavFile.existsAsFile())
        return false;

    convolution.loadImpulseResponse (wavFile,
                                     juce::dsp::Convolution::Stereo::no,
                                     juce::dsp::Convolution::Trim::yes,
                                     0,  // usa o IR inteiro
                                     juce::dsp::Convolution::Normalise::yes);
    irFile = wavFile;
    irActive = true;
    return true;
}

void Cabinet::clearImpulseResponse()
{
    irActive = false;
    irFile = juce::File();
}

void Cabinet::process (float* data, int numSamples) noexcept
{
    if (irActive.load() && convolution.getCurrentIRSize() > 0)
    {
        float* channels[] = { data };
        juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);
        convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float s = hp.processSample (data[i]);
        s = notch.processSample (s);
        s = peak.processSample (s);
        s = lp2.processSample (lp1.processSample (s));
        data[i] = s;
    }
}
