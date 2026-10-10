#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Colours
{
    const juce::Colour background { 0xff17140f };
    const juce::Colour panel      { 0xff241f18 };
    const juce::Colour wood       { 0xff3a2a1c };
    const juce::Colour amber      { 0xffe8a33d };  // cor da marca Wilbo Studio
    const juce::Colour text       { 0xfff2e8d8 };
    const juce::Colour textDim    { 0xffa89a85 };
    const juce::Colour ledOn      { 0xffff4a3d };
    const juce::Colour ledOff     { 0xff4a1e1a };
    const juce::Colour good       { 0xff5fd36a };
}

// Desenho customizado: knobs de pedal, footswitch com LED, etc.
class WilboLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WilboLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
};
