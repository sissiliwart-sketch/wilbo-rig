#pragma once

#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/PedalComponent.h"
#include "UI/TunerDisplay.h"

// Medidor de nivel simples (verde -> amarelo -> vermelho)
class LevelMeter : public juce::Component
{
public:
    void setLevel (float linear) { level = linear; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float level = 0.0f;
};

// Painel de um bloco (amp / gabinete) com titulo
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::String t, juce::Colour c) : title (std::move (t)), colour (c) {}
    void paint (juce::Graphics&) override;
    juce::Rectangle<int> getContentArea() const { return getLocalBounds().reduced (14).withTrimmedTop (30); }
private:
    juce::String title;
    juce::Colour colour;
};

class WilboRigEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WilboRigEditor (WilboRigProcessor&);
    ~WilboRigEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseImpulseResponse();
    void updateIrLabel();

    WilboRigProcessor& rig;
    WilboLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // Barra superior
    Knob inputKnob, gateKnob, outputKnob;
    juce::ToggleButton gateButton { "Noise Gate" };
    ButtonAttachment gateAttachment;
    juce::TextButton tunerButton { "AFINADOR" };
    ButtonAttachment tunerAttachment;
    TunerDisplay tunerDisplay;
    LevelMeter inMeter, outMeter;

    // Pedais
    PedalComponent wahPedal, drivePedal, delayPedal, reverbPedal;
    juce::ToggleButton wahAutoButton { "Auto-wah" };
    ButtonAttachment wahAutoAttachment;
    juce::ComboBox driveTypeBox;
    std::unique_ptr<ComboAttachment> driveTypeAttachment;

    // Amp + gabinete
    SectionPanel ampPanel { "AMPLIFICADOR", Colours::amber }, cabPanel { "GABINETE", Colours::amber };
    juce::ToggleButton ampOnButton { "Ligado" }, cabOnButton { "Ligado" };
    ButtonAttachment ampOnAttachment, cabOnAttachment;
    std::vector<std::unique_ptr<Knob>> ampKnobs;
    juce::TextButton loadIrButton { "Carregar IR (.wav)" }, clearIrButton { "Usar simulador interno" };
    juce::Label irLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WilboRigEditor)
};
