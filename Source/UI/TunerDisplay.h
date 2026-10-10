#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/Tuner.h"

// Mostra a nota, a frequencia e um ponteiro de -50 a +50 cents.
class TunerDisplay : public juce::Component, private juce::Timer
{
public:
    explicit TunerDisplay (Tuner& t);
    void paint (juce::Graphics&) override;
    void setActive (bool shouldBeActive) { active = shouldBeActive; repaint(); }

private:
    void timerCallback() override;

    Tuner& tuner;
    Tuner::Result result;
    bool active = false;
    int framesWithoutSignal = 0;
};
