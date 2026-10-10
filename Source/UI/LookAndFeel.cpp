#include "LookAndFeel.h"

WilboLookAndFeel::WilboLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha (0.35f));
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black.withAlpha (0.5f));
    setColour (juce::ComboBox::arrowColourId, Colours::text);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panel);
    setColour (juce::PopupMenu::textColourId, Colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::amber);
    setColour (juce::TextButton::textColourOffId, Colours::text);
    setColour (juce::TextButton::textColourOnId, Colours::background);
    setColour (juce::BubbleComponent::backgroundColourId, Colours::panel);
    setColour (juce::BubbleComponent::outlineColourId, Colours::amber);
    setColour (juce::TooltipWindow::textColourId, Colours::text);
}

void WilboLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // arco de valor
    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corpo do knob (preto com borda, estilo "chicken head" simplificado)
    const float knobR = radius * 0.74f;
    juce::ColourGradient grad (juce::Colour (0xff3b3833), centre.x, centre.y - knobR,
                               juce::Colour (0xff0e0d0c), centre.x, centre.y + knobR, false);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2, knobR * 2);
    g.setColour (juce::Colours::black);
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2, knobR * 2, 1.2f);

    // ponteiro
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR, 3.0f, knobR * 0.55f, 1.5f);
    g.setColour (Colours::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void WilboLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat();

    if (b.getProperties()["footswitch"])
    {
        // LED em cima
        const float led = 10.0f;
        auto ledArea = juce::Rectangle<float> (led, led).withCentre ({ r.getCentreX(), r.getY() + led });
        if (on)
        {
            g.setColour (Colours::ledOn.withAlpha (0.35f));
            g.fillEllipse (ledArea.expanded (5.0f));
        }
        g.setColour (on ? Colours::ledOn : Colours::ledOff);
        g.fillEllipse (ledArea);

        // botao metalico
        const float s = juce::jmin (r.getWidth(), r.getHeight() - led * 2.5f) * 0.9f;
        auto sw = juce::Rectangle<float> (s, s).withCentre ({ r.getCentreX(), r.getBottom() - s * 0.5f - 2.0f });
        juce::ColourGradient grad (juce::Colour (0xffe6e3dc), sw.getX(), sw.getY(),
                                   juce::Colour (0xff6f6b64), sw.getRight(), sw.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (down ? sw.reduced (1.5f) : sw);
        g.setColour (juce::Colours::black.withAlpha (highlighted ? 0.8f : 0.55f));
        g.drawEllipse (sw, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (sw.reduced (s * 0.28f));
        return;
    }

    // Toggle pequeno (pilula)
    auto pill = r.removeFromLeft (34.0f).withSizeKeepingCentre (30.0f, 16.0f);
    g.setColour (on ? Colours::amber : juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (pill, 8.0f);
    g.setColour (Colours::text);
    const float knobX = on ? pill.getRight() - 14.0f : pill.getX() + 2.0f;
    g.fillEllipse (knobX, pill.getY() + 2.0f, 12.0f, 12.0f);

    g.setColour (highlighted ? Colours::text : Colours::textDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 1);
}

void WilboLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                             bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? Colours::amber : juce::Colours::black.withAlpha (down ? 0.55f : (highlighted ? 0.45f : 0.3f)));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Colours::amber.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

