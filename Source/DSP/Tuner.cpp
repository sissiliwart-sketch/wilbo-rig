#include "Tuner.h"
#include <algorithm>
#include <cmath>

void Tuner::prepare (double sampleRate)
{
    decimation    = juce::jmax (1, (int) std::round (sampleRate / 24000.0));
    decimatedRate = sampleRate / decimation;
    decimCount = 0;
    decimAccum = 0.0f;
    fifo.reset();
    smoothedFreq = 0.0f;
}

void Tuner::writeToFifo (const float* src, int count) noexcept
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite (count, s1, n1, s2, n2);   // se a fila encher, o excesso e descartado
    std::copy_n (src, n1, fifoBuffer.begin() + s1);
    std::copy_n (src + n1, n2, fifoBuffer.begin() + s2);
    fifo.finishedWrite (n1 + n2);
}

void Tuner::push (const float* data, int numSamples) noexcept
{
    // Reduz a taxa de amostragem (media simples funciona como um filtro passa-baixa)
    float local[512];
    int n = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        decimAccum += data[i];

        if (++decimCount >= decimation)
        {
            local[n++] = decimAccum / (float) decimation;
            decimAccum = 0.0f;
            decimCount = 0;

            if (n == (int) std::size (local))
            {
                writeToFifo (local, n);
                n = 0;
            }
        }
    }

    if (n > 0)
        writeToFifo (local, n);
}

float Tuner::yin (const float* x, int size, double sampleRate, float threshold)
{
    const int maxTau = size / 2;
    if (maxTau < 4)
        return 0.0f;

    // Ignora sinal fraco demais (silencio / chiado)
    float energy = 0.0f;
    for (int i = 0; i < size; ++i)
        energy += x[i] * x[i];
    if (std::sqrt (energy / (float) size) < 0.002f)
        return 0.0f;

    std::vector<float> d ((size_t) maxTau, 0.0f);
    std::vector<float> raw ((size_t) maxTau, 0.0f);

    // 1) funcao diferenca
    for (int tau = 1; tau < maxTau; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < maxTau; ++i)
        {
            const float delta = x[i] - x[i + tau];
            sum += delta * delta;
        }
        d[(size_t) tau] = sum;
        raw[(size_t) tau] = sum;
    }

    // 2) diferenca normalizada cumulativa
    d[0] = 1.0f;
    float running = 0.0f;
    for (int tau = 1; tau < maxTau; ++tau)
    {
        running += d[(size_t) tau];
        d[(size_t) tau] = running > 0.0f ? d[(size_t) tau] * (float) tau / running : 1.0f;
    }

    // 3) primeiro minimo abaixo do limiar
    const int minTau = juce::jmax (2, (int) (sampleRate / 1400.0)); // ate ~1400 Hz
    int tauEstimate = -1;

    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (d[(size_t) tau] < threshold)
        {
            while (tau + 1 < maxTau && d[(size_t) tau + 1] < d[(size_t) tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate < 0)
        return 0.0f;

    // 4) interpolacao parabolica (na funcao diferenca original, que nao tem vies)
    //    para precisao abaixo de 1 amostra
    float betterTau = (float) tauEstimate;
    if (tauEstimate > 1 && tauEstimate < maxTau - 1)
    {
        const float s0 = raw[(size_t) tauEstimate - 1];
        const float s1 = raw[(size_t) tauEstimate];
        const float s2 = raw[(size_t) tauEstimate + 1];
        const float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs (denom) > 1.0e-9f)
            betterTau += (s2 - s0) / denom;
    }

    return (float) (sampleRate / betterTau);
}

Tuner::Result Tuner::detect()
{
    // Puxa tudo o que chegou e mantem as ultimas 'windowSize' amostras
    const int ready = fifo.getNumReady();
    if (ready > 0)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (ready, s1, n1, s2, n2);
        std::copy_n (fifoBuffer.begin() + s1, n1, scratch.begin());
        std::copy_n (fifoBuffer.begin() + s2, n2, scratch.begin() + n1);
        fifo.finishedRead (n1 + n2);

        if (ready >= windowSize)
        {
            std::copy (scratch.begin() + (ready - windowSize), scratch.begin() + ready, window.begin());
        }
        else
        {
            std::move (window.begin() + ready, window.end(), window.begin());
            std::copy (scratch.begin(), scratch.begin() + ready, window.end() - ready);
        }
    }

    Result r;
    const float f = yin (window.data(), windowSize, decimatedRate);

    if (f < 30.0f || f > 1400.0f)
    {
        smoothedFreq = 0.0f;
        return r;
    }

    // Suaviza o ponteiro, mas pula direto se a nota mudou muito
    if (smoothedFreq <= 0.0f || std::abs (std::log2 (f / smoothedFreq)) > 0.03f)
        smoothedFreq = f;
    else
        smoothedFreq = 0.7f * smoothedFreq + 0.3f * f;

    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    const float noteFloat = 69.0f + 12.0f * std::log2 (smoothedFreq / referenceA4);
    const int note = (int) std::round (noteFloat);

    r.frequency = smoothedFreq;
    r.midiNote  = note;
    r.cents     = (noteFloat - (float) note) * 100.0f;
    r.noteName  = names[((note % 12) + 12) % 12];
    r.octave    = note / 12 - 1;
    return r;
}
