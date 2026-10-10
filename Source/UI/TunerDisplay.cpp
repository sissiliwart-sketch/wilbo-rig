#include "TunerDisplay.h"
#include "LookAndFeel.h"

TunerDisplay::TunerDisplay (Tuner& t) : tuner (t)
{
    startTimerHz (20);
}

void TunerDisplay::timerCallback()
{
    auto r = tuner.detect();

    if (r.frequency > 0.0f)
    {
        result = r;
        framesWithoutSignal = 0;
    }
    else if (++framesWithoutSignal > 15)  // segura a ultima nota por ~0,75 s
    {
        result = {};
    }

    repaint();
}

void TunerDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour ((active ? Colours::amber : Colours::textDim).withAlpha (0.6f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    auto area = r.reduced (10.0f, 6.0f);
    auto noteArea = area.removeFromLeft (70.0f);

    const bool hasNote = result.frequency > 0.0f;
    const bool inTune = hasNote && std::abs (result.cents) < 3.0f;
    const auto noteColour = ! hasNote ? Colours::textDim : (inTune ? Colours::good : Colours::text);

    g.setColour (noteColour);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText (hasNote ? result.noteName : "-", noteArea.removeFromTop (noteArea.getHeight() * 0.72f),
                juce::Justification::centred);
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (Colours::textDim);
    g.drawText (hasNote ? juce::String (result.frequency, 1) + " Hz" : (active ? "toque uma corda" : "AFINADOR"),
                noteArea, juce::Justification::centred);

    // escala de cents
    auto scale = area.reduced (8.0f, area.getHeight() * 0.25f);
    const float cx = scale.getCentreX();
    g.setColour (Colours::textDim.withAlpha (0.5f));
    for (int c = -50; c <= 50; c += 10)
    {
        const float x = cx + (float) c / 50.0f * scale.getWidth() * 0.5f;
        const float h = c == 0 ? scale.getHeight() : scale.getHeight() * 0.5f;
        g.fillRect (x - 0.5f, scale.getCentreY() - h * 0.5f, 1.0f, h);
    }

    if (hasNote)
    {
        const float x = cx + juce::jlimit (-50.0f, 50.0f, result.cents) / 50.0f * scale.getWidth() * 0.5f;
        g.setColour (inTune ? Colours::good : Colours::amber);
        g.fillRoundedRectangle (x - 3.0f, scale.getY() - 4.0f, 6.0f, scale.getHeight() + 8.0f, 3.0f);

        g.setFont (juce::FontOptions (11.0f));
        g.drawText ((result.cents >= 0 ? "+" : "") + juce::String ((int) std::round (result.cents)) + " cents",
                    area.removeFromBottom (14.0f), juce::Justification::centred);
    }
}
