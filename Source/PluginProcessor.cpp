#include "PluginProcessor.h"
#include "PluginEditor.h"

WilboRigProcessor::WilboRigProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WilboRig", createParameterLayout())
{
}

bool WilboRigProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void WilboRigProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) samplesPerBlock, 1 };

    gate.prepare (monoSpec);
    gate.setRatio (10.0f);
    gate.setAttack (1.0f);
    gate.setRelease (80.0f);

    tuner.prepare (sampleRate);
    wah.prepare (sampleRate, samplesPerBlock);
    drive.prepare (sampleRate, samplesPerBlock);
    amp.prepare (sampleRate, samplesPerBlock);
    cabinet.prepare (sampleRate, samplesPerBlock);
    delay.prepare (sampleRate, samplesPerBlock);
    reverb.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });

    inGainSmoothed.reset (sampleRate, 0.03);
    outGainSmoothed.reset (sampleRate, 0.03);
    monoBuffer.setSize (1, samplesPerBlock);
}

// Limitador suave: nao mexe no sinal abaixo de 0,8 e arredonda os picos acima,
// em vez de cortar seco (o corte seco soa como "estalo").
static inline float softLimit (float x) noexcept
{
    constexpr float knee = 0.8f;
    const float a = std::abs (x);
    if (a <= knee)
        return x;
    const float over = (a - knee) / (1.0f - knee);
    return std::copysign (knee + (1.0f - knee) * std::tanh (over), x);
}

void WilboRigProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    using namespace ParamID;

    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    if (monoBuffer.getNumSamples() < n)
        monoBuffer.setSize (1, n, false, false, true);

    // 1) A guitarra entra pelo canal 1 (esquerdo) da interface
    float* x = monoBuffer.getWritePointer (0);
    if (numIn > 0)
        juce::FloatVectorOperations::copy (x, buffer.getReadPointer (0), n);
    else
        juce::FloatVectorOperations::clear (x, n);

    inGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (p (inGain)));
    inGainSmoothed.applyGain (x, n);
    inputLevel = juce::jmax (inputLevel.load() * 0.9f, juce::FloatVectorOperations::findMaximum (x, n),
                             -juce::FloatVectorOperations::findMinimum (x, n));

    float* channels[] = { x };
    juce::dsp::AudioBlock<float> monoBlock (channels, 1, (size_t) n);

    // 2) Noise gate: corta o chiado quando voce nao esta tocando
    if (on (gateOn))
    {
        gate.setThreshold (p (gateThresh));
        gate.process (juce::dsp::ProcessContextReplacing<float> (monoBlock));
    }

    // 3) Afinador sempre "escuta" o sinal limpo
    tuner.push (x, n);

    // 4) Pedais
    if (on (wahOn))
    {
        wah.setAuto (on (wahAuto));
        wah.setPedal (p (wahPedal));
        wah.setQ (p (wahQ));
        wah.setSensitivity (p (wahSens));
        wah.process (x, n);
        wahPosition = wah.getCurrentPosition();
    }

    if (on (driveOn))
    {
        drive.setType ((int) p (driveType));
        drive.setGain (p (driveGain));
        drive.setTone (p (driveTone));
        drive.setLevelDb (p (driveLevel));
        drive.process (x, n);
    }

    if (on (ampOn))
    {
        amp.setSettings ({ p (ampGain), p (ampBass), p (ampMid), p (ampTreble), p (ampPresence), p (ampMaster) });
        amp.process (x, n);
    }

    if (on (cabOn))
        cabinet.process (x, n);

    if (on (delayOn))
    {
        delay.setTimeMs (p (delayTime));
        delay.setFeedback (p (delayFb));
        delay.setTone (p (delayTone));
        delay.setMix (p (delayMix));
        delay.process (x, n);
    }

    // 5) Copia o mono para as saidas (L e R)
    for (int ch = 0; ch < numOut; ++ch)
        buffer.copyFrom (ch, 0, x, n);
    for (int ch = numOut; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);

    // 6) Reverb em estereo
    if (on (reverbOn) && numOut > 0)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = p (reverbSize);
        rp.damping  = p (reverbDamp);
        rp.wetLevel = p (reverbMix);
        rp.dryLevel = 1.0f - 0.5f * p (reverbMix);
        rp.width    = 1.0f;
        reverb.setParameters (rp);

        juce::dsp::AudioBlock<float> outBlock (buffer.getArrayOfWritePointers(), (size_t) juce::jmin (numOut, 2), (size_t) n);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (outBlock));
    }

    // 7) Volume final (e mudo enquanto o afinador estiver ligado)
    outGainSmoothed.setTargetValue (on (tunerOn) ? 0.0f : juce::Decibels::decibelsToGain (p (outGain)));
    float peak = 0.0f;
    for (int ch = 0; ch < numOut; ++ch)
    {
        auto gainCopy = outGainSmoothed;  // mesma rampa em todos os canais
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            d[i] = softLimit (d[i] * gainCopy.getNextValue()); // protecao contra picos
            peak = juce::jmax (peak, std::abs (d[i]));
        }
        if (ch == numOut - 1)
            outGainSmoothed = gainCopy;
    }
    outputLevel = juce::jmax (outputLevel.load() * 0.9f, peak);
}

juce::AudioProcessorEditor* WilboRigProcessor::createEditor()
{
    return new WilboRigEditor (*this);
}

void WilboRigProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", cabinet.getIrFile().getFullPathName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void WilboRigProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.hasType (apvts.state.getType()))
        {
            const juce::String ir = state.getProperty ("irPath").toString();
            apvts.replaceState (state);
            if (ir.isNotEmpty())
                cabinet.loadImpulseResponse (juce::File (ir));
        }
    }
}

// Ponto de entrada exigido pelo JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WilboRigProcessor();
}
