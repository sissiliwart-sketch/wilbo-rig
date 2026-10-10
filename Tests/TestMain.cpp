// Testes offline: rodam o processamento sem placa de som.
//   cmake -B build -DWILBO_BUILD_TESTS=ON && cmake --build build --target WilboTests
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>
#include <iostream>

static int failures = 0;
#define CHECK(cond, msg) do { if (cond) std::cout << "  ok   " << msg << "\n"; \
                              else { std::cout << "  FALHOU " << msg << "\n"; ++failures; } } while (0)

// Corda sintetica (Karplus-Strong) para simular uma guitarra
static std::vector<float> pluck (float freq, double sr, int numSamples, float amp = 0.4f, int seed = 1)
{
    juce::Random rng (seed);
    const int period = juce::jmax (2, (int) std::round (sr / freq));
    std::vector<float> ring ((size_t) period);
    for (auto& s : ring) s = rng.nextFloat() * 2.0f - 1.0f;

    std::vector<float> out ((size_t) numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        const size_t a = (size_t) (i % period), b = (size_t) ((i + 1) % period);
        out[(size_t) i] = ring[a] * amp;
        ring[a] = 0.497f * (ring[a] + ring[b]);
    }
    return out;
}

static bool allFinite (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}

static void setParam (WilboRigProcessor& p, const char* id, float realValue)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (realValue));
}

// Processa um sinal mono em blocos, como a placa de som faria
static juce::AudioBuffer<float> run (WilboRigProcessor& p, const std::vector<float>& in, int block = 256)
{
    const int n = (int) in.size();
    juce::AudioBuffer<float> out (2, n);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;

    for (int pos = 0; pos < n; pos += block)
    {
        const int len = juce::jmin (block, n - pos);
        buf.setSize (2, len, false, false, true);
        buf.copyFrom (0, 0, in.data() + pos, len);
        buf.copyFrom (1, 0, in.data() + pos, len);
        p.processBlock (buf, midi);
        out.copyFrom (0, pos, buf, 0, 0, len);
        out.copyFrom (1, pos, buf, 1, 0, len);
    }
    return out;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const double sr = 48000.0;

    std::cout << "\n[1] Afinador (YIN) com cordas soltas\n";
    {
        const float strings[] = { 82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f };
        const char* names[]   = { "E", "A", "D", "G", "B", "E" };

        for (int s = 0; s < 6; ++s)
        {
            Tuner tuner;
            tuner.prepare (sr);
            auto sig = pluck (strings[s], sr, (int) sr / 2, 0.4f, s + 3);
            // Karplus-Strong: y[n] = media(y[n-N], y[n-N+1]) -> periodo = N - 0,5 amostras
            const float actual = (float) (sr / (std::round (sr / strings[s]) - 0.5));

            Tuner::Result r;
            for (int pos = 0; pos + 256 <= (int) sig.size(); pos += 256)
            {
                tuner.push (sig.data() + pos, 256);
                if (pos % 2400 == 0) r = tuner.detect();
            }
            r = tuner.detect();
            const float centsErr = 1200.0f * std::log2 (r.frequency / actual);
            CHECK (r.noteName == names[s] && std::abs (centsErr) < 3.0f,
                   juce::String::formatted ("corda %s: esperado %.2f Hz, detectado %.2f Hz (%s, erro %.2f cents)",
                                            names[s], actual, r.frequency, r.noteName.toRawUTF8(), centsErr));
        }
    }

    std::cout << "\n[2] Cadeia completa sem NaN/estouro\n";
    WilboRigProcessor proc;
    proc.setPlayConfigDetails (2, 2, sr, 256);
    proc.prepareToPlay (sr, 256);

    std::vector<float> riff;
    for (float f : { 82.41f, 110.0f, 146.83f, 110.0f })
    {
        auto note = pluck (f, sr, (int) (sr * 0.5), 0.5f);
        riff.insert (riff.end(), note.begin(), note.end());
    }
    riff.resize (riff.size() + (size_t) sr, 0.0f);  // 1 s de silencio no fim (cauda)

    using namespace ParamID;
    for (auto id : { wahOn, driveOn, ampOn, cabOn, delayOn, reverbOn, gateOn })
        setParam (proc, id, 1.0f);

    for (int type = 0; type < 3; ++type)
    {
        setParam (proc, driveType, (float) type);
        setParam (proc, driveGain, 1.0f);
        setParam (proc, ampGain, 10.0f);
        auto out = run (proc, riff);
        const float peak = out.getMagnitude (0, out.getNumSamples());
        CHECK (allFinite (out) && peak > 0.01f && peak <= 1.0f,
               juce::String::formatted ("drive tipo %d + amp no talo: pico %.3f", type, peak));
    }

    // auto-wah e tamanhos de bloco irregulares (algumas placas fazem isso)
    setParam (proc, wahAuto, 1.0f);
    {
        auto out = run (proc, riff, 137);
        CHECK (allFinite (out), "auto-wah com bloco de 137 amostras");
    }

    std::cout << "\n[3] Delay gera repeticao\n";
    {
        for (auto id : { wahOn, driveOn, ampOn, cabOn, reverbOn, gateOn })
            setParam (proc, id, 0.0f);
        setParam (proc, delayTime, 250.0f);
        setParam (proc, delayMix, 0.5f);
        setParam (proc, delayFb, 0.0f);
        proc.prepareToPlay (sr, 256);

        std::vector<float> click ((size_t) sr, 0.0f);
        for (int i = 0; i < 48; ++i) click[(size_t) i] = 0.5f;
        auto out = run (proc, click);
        const float echo = out.getMagnitude (0, (int) (0.25 * sr) - 200, 600);
        const float gap  = out.getMagnitude (0, (int) (0.10 * sr), 2000);
        CHECK (echo > 0.05f && gap < 1.0e-3f,
               juce::String::formatted ("eco em 250 ms (nivel %.3f, silencio antes %.5f)", echo, gap));
    }

    std::cout << "\n[4] Afinador silencia a saida\n";
    {
        setParam (proc, tunerOn, 1.0f);
        auto out = run (proc, riff);
        const float tailPeak = out.getMagnitude (0, out.getNumSamples() / 2, out.getNumSamples() / 2);
        CHECK (tailPeak < 1.0e-4f, juce::String::formatted ("saida com afinador ligado: %.6f", tailPeak));
        setParam (proc, tunerOn, 0.0f);
    }

    std::cout << "\n[5] Salvar / restaurar preset\n";
    {
        setParam (proc, ampGain, 7.5f);
        juce::MemoryBlock state;
        proc.getStateInformation (state);
        WilboRigProcessor other;
        other.setStateInformation (state.getData(), (int) state.getSize());
        const float g = other.apvts.getRawParameterValue (ampGain)->load();
        CHECK (std::abs (g - 7.5f) < 0.02f, juce::String::formatted ("amp gain restaurado: %.2f", g));
    }

    std::cout << "\n[6] Gerando demo.wav (riff com o timbre padrao + drive)\n";
    {
        WilboRigProcessor demo;
        demo.setPlayConfigDetails (2, 2, sr, 256);
        demo.prepareToPlay (sr, 256);
        setParam (demo, driveOn, 1.0f);
        setParam (demo, driveGain, 0.6f);
        setParam (demo, delayOn, 1.0f);
        auto out = run (demo, riff);

        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile ("demo.wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
        CHECK (w != nullptr, "demo.wav escrito em " + f.getFullPathName());

        const float peak = out.getMagnitude (0, out.getNumSamples());
        const float rms  = out.getRMSLevel (0, 0, out.getNumSamples());
        CHECK (peak < 0.95f && rms > 0.03f,
               juce::String::formatted ("volume com ajustes padrao: pico %.2f, RMS %.3f (sem saturar a saida)", peak, rms));
    }

    std::cout << "\n" << (failures == 0 ? "TODOS OS TESTES PASSARAM" : "HA FALHAS") << "\n";
    return failures == 0 ? 0 : 1;
}
