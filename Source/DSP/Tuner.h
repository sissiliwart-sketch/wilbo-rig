#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

// Afinador cromatico com o algoritmo YIN.
//
// - A thread de audio chama push(): o sinal e reduzido para ~24 kHz
//   (media de N amostras) e colocado numa fila sem bloqueio (AbstractFifo).
// - A interface chama detect() umas 20 vezes por segundo; o calculo pesado
//   acontece fora da thread de audio, entao nao causa estalos.
class Tuner
{
public:
    struct Result
    {
        float frequency = 0.0f;   // Hz (0 = nada detectado)
        int   midiNote  = -1;     // 69 = A4
        float cents     = 0.0f;   // -50 .. +50
        juce::String noteName;    // "E", "A#"...
        int   octave    = 0;
    };

    void prepare (double sampleRate);
    void push (const float* data, int numSamples) noexcept;  // thread de audio
    Result detect();                                         // thread da interface

    void setReferenceA4 (float hz) noexcept { referenceA4 = hz; }

    // Exposto para os testes
    static float yin (const float* x, int size, double sampleRate, float threshold = 0.12f);

private:
    void writeToFifo (const float* src, int count) noexcept;

    static constexpr int fifoSize   = 8192;
    static constexpr int windowSize = 2048;   // amostras ja reduzidas (~85 ms a 24 kHz)

    juce::AbstractFifo fifo { fifoSize };
    std::vector<float> fifoBuffer = std::vector<float> (fifoSize, 0.0f);

    std::vector<float> window = std::vector<float> (windowSize, 0.0f);
    std::vector<float> scratch = std::vector<float> (fifoSize, 0.0f);

    int   decimation = 4;
    int   decimCount = 0;
    float decimAccum = 0.0f;
    double decimatedRate = 24000.0;

    float referenceA4 = 440.0f;
    float smoothedFreq = 0.0f;
};
