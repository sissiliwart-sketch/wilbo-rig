#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"

// Um knob com nome embaixo, ligado a um parametro.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
          const juce::String& labelText, juce::Colour accent,
          juce::Colour labelColour = Colours::text);

    void resized() override;

    juce::Slider slider;

private:
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// Um pedal: caixa colorida com titulo, knobs, controles extras e footswitch.
class PedalComponent : public juce::Component
{
public:
    struct KnobSpec { juce::String paramId, label; };

    PedalComponent (juce::AudioProcessorValueTreeState& state, const juce::String& title,
                    juce::Colour body, const juce::String& footswitchId,
                    const std::vector<KnobSpec>& knobs, int knobsPerRow = 2);

    // controles extras (ex.: seletor de tipo, botao "Auto") ficam acima do footswitch
    void addExtra (juce::Component& c, int height = 26);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String title;
    juce::Colour body;
    int knobsPerRow;

    juce::ToggleButton footswitch;
    juce::AudioProcessorValueTreeState::ButtonAttachment footswitchAttachment;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::pair<juce::Component*, int>> extras;
};
