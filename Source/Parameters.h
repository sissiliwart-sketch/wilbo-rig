#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Todos os IDs de parametros ficam aqui, num lugar so.
// Para adicionar um novo knob: crie o ID aqui, registre em Parameters.cpp
// e leia o valor no PluginProcessor.
namespace ParamID
{
    // Entrada / saida / gate
    inline constexpr auto inGain      = "inGain";
    inline constexpr auto outGain     = "outGain";
    inline constexpr auto gateOn      = "gateOn";
    inline constexpr auto gateThresh  = "gateThresh";

    // Afinador
    inline constexpr auto tunerOn     = "tunerOn";   // liga o afinador e silencia a saida

    // Wah
    inline constexpr auto wahOn       = "wahOn";
    inline constexpr auto wahAuto     = "wahAuto";   // false = pedal manual, true = auto-wah
    inline constexpr auto wahPedal    = "wahPedal";
    inline constexpr auto wahQ        = "wahQ";
    inline constexpr auto wahSens     = "wahSens";

    // Drive
    inline constexpr auto driveOn     = "driveOn";
    inline constexpr auto driveType   = "driveType"; // 0 Overdrive, 1 Distortion, 2 Fuzz
    inline constexpr auto driveGain   = "driveGain";
    inline constexpr auto driveTone   = "driveTone";
    inline constexpr auto driveLevel  = "driveLevel";

    // Amplificador
    inline constexpr auto ampOn       = "ampOn";
    inline constexpr auto ampGain     = "ampGain";
    inline constexpr auto ampBass     = "ampBass";
    inline constexpr auto ampMid      = "ampMid";
    inline constexpr auto ampTreble   = "ampTreble";
    inline constexpr auto ampPresence = "ampPresence";
    inline constexpr auto ampMaster   = "ampMaster";

    // Gabinete
    inline constexpr auto cabOn       = "cabOn";

    // Delay
    inline constexpr auto delayOn     = "delayOn";
    inline constexpr auto delayTime   = "delayTime";
    inline constexpr auto delayFb     = "delayFb";
    inline constexpr auto delayTone   = "delayTone";
    inline constexpr auto delayMix    = "delayMix";

    // Reverb
    inline constexpr auto reverbOn    = "reverbOn";
    inline constexpr auto reverbSize  = "reverbSize";
    inline constexpr auto reverbDamp  = "reverbDamp";
    inline constexpr auto reverbMix   = "reverbMix";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
