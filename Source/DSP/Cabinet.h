#pragma once

#include <juce_dsp/juce_dsp.h>

// Simulador de gabinete.
// - Com um arquivo IR (.wav) carregado: convolucao (som mais realista).
// - Sem IR: um filtro que imita a resposta de uma caixa 4x12 (corta graves
//   muito baixos e agudos acima de ~5 kHz, que soam como "chiado" de lixa).
class Cabinet
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (float* data, int numSamples) noexcept;

    // Chamar da thread da interface. Retorna false se o arquivo nao abrir.
    bool loadImpulseResponse (const juce::File& wavFile);
    void clearImpulseResponse();

    juce::File getIrFile() const { return irFile; }
    bool hasImpulseResponse() const noexcept { return irActive.load(); }

private:
    juce::dsp::Convolution convolution;
    juce::dsp::IIR::Filter<float> hp, lp1, lp2, peak, notch;

    std::atomic<bool> irActive { false };
    juce::File irFile;
    double sr = 48000.0;
};
