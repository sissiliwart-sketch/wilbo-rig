#include "Parameters.h"

namespace
{
    using Range = juce::NormalisableRange<float>;

    std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const char* name,
                                                     Range range, float def,
                                                     const char* unit = "")
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def,
            juce::AudioParameterFloatAttributes().withLabel (unit));
    }

    std::unique_ptr<juce::AudioParameterBool> toggle (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    }

    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const Range zeroTen (0.0f, 10.0f, 0.01f);
    const Range unit (0.0f, 1.0f, 0.001f);

    // Entrada / saida / gate
    layout.add (knob (inGain,     "Input",          { -12.0f, 24.0f, 0.1f }, 0.0f,   "dB"));
    layout.add (knob (outGain,    "Output",         { -40.0f, 12.0f, 0.1f }, -6.0f,  "dB"));
    layout.add (toggle (gateOn,   "Gate On", true));
    layout.add (knob (gateThresh, "Gate Threshold", { -90.0f, -20.0f, 0.1f }, -62.0f, "dB"));

    layout.add (toggle (tunerOn,  "Tuner", false));

    // Wah
    layout.add (toggle (wahOn,    "Wah On", false));
    layout.add (toggle (wahAuto,  "Wah Auto", false));
    layout.add (knob (wahPedal,   "Wah Pedal", unit, 0.5f));
    layout.add (knob (wahQ,       "Wah Q",     { 1.0f, 12.0f, 0.01f }, 5.0f));
    layout.add (knob (wahSens,    "Wah Sens",  unit, 0.5f));

    // Drive
    layout.add (toggle (driveOn,  "Drive On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { driveType, 1 }, "Drive Type",
        juce::StringArray { "Overdrive", "Distortion", "Fuzz" }, 0));
    layout.add (knob (driveGain,  "Drive",       unit, 0.5f));
    layout.add (knob (driveTone,  "Drive Tone",  unit, 0.5f));
    layout.add (knob (driveLevel, "Drive Level", { -24.0f, 12.0f, 0.1f }, 0.0f, "dB"));

    // Amp
    layout.add (toggle (ampOn,    "Amp On", true));
    layout.add (knob (ampGain,    "Amp Gain",   zeroTen, 4.0f));
    layout.add (knob (ampBass,    "Bass",       zeroTen, 5.0f));
    layout.add (knob (ampMid,     "Mid",        zeroTen, 5.0f));
    layout.add (knob (ampTreble,  "Treble",     zeroTen, 5.0f));
    layout.add (knob (ampPresence,"Presence",   zeroTen, 5.0f));
    layout.add (knob (ampMaster,  "Master",     { -30.0f, 6.0f, 0.1f }, -6.0f, "dB"));

    layout.add (toggle (cabOn,    "Cab On", true));

    // Delay
    layout.add (toggle (delayOn,  "Delay On", false));
    layout.add (knob (delayTime,  "Delay Time", skewed (20.0f, 2000.0f, 400.0f), 380.0f, "ms"));
    layout.add (knob (delayFb,    "Feedback",   { 0.0f, 0.95f, 0.001f }, 0.35f));
    layout.add (knob (delayTone,  "Delay Tone", unit, 0.6f));
    layout.add (knob (delayMix,   "Delay Mix",  unit, 0.3f));

    // Reverb
    layout.add (toggle (reverbOn, "Reverb On", true));
    layout.add (knob (reverbSize, "Room Size",  unit, 0.45f));
    layout.add (knob (reverbDamp, "Damping",    unit, 0.5f));
    layout.add (knob (reverbMix,  "Reverb Mix", unit, 0.18f));

    return layout;
}
