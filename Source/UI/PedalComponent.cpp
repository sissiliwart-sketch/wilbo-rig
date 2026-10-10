#include "PedalComponent.h"

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
            const juce::String& labelText, juce::Colour accent, juce::Colour labelColour)
    : attachment (state, paramId, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);   // mostra o valor ao girar
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setDoubleClickReturnValue (true, state.getParameter (paramId)->convertFrom0to1 (
                                                state.getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (slider);

    label.setText (labelText.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, labelColour);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void Knob::resized()
{
    // knob quadrado com o nome logo abaixo, centralizados no espaco disponivel
    auto r = getLocalBounds();
    const int size = juce::jmin (r.getWidth(), r.getHeight() - 16, 110);
    auto block = r.withSizeKeepingCentre (r.getWidth(), size + 16);
    slider.setBounds (block.removeFromTop (size).withSizeKeepingCentre (size, size));
    label.setBounds (block);
}

//==============================================================================
PedalComponent::PedalComponent (juce::AudioProcessorValueTreeState& state, const juce::String& t,
                                juce::Colour b, const juce::String& footswitchId,
                                const std::vector<KnobSpec>& specs, int perRow)
    : title (t), body (b), knobsPerRow (perRow),
      footswitchAttachment (state, footswitchId, footswitch)
{
    footswitch.getProperties().set ("footswitch", true);
    footswitch.setTooltip ("Liga / desliga " + title);
    addAndMakeVisible (footswitch);

    const auto accent = body.getBrightness() > 0.6f ? juce::Colours::black : Colours::text;

    for (auto& s : specs)
    {
        auto k = std::make_unique<Knob> (state, s.paramId, s.label, accent.withAlpha (0.85f), accent);
        addAndMakeVisible (*k);
        knobs.push_back (std::move (k));
    }
}

void PedalComponent::addExtra (juce::Component& c, int height)
{
    extras.push_back ({ &c, height });
    addAndMakeVisible (c);
    resized();
}

void PedalComponent::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);

    // sombra + corpo
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 3), 10.0f);

    juce::ColourGradient grad (body.brighter (0.15f), r.getX(), r.getY(),
                               body.darker (0.35f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 10.0f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 10.0f, 1.5f);

    // parafusos
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (auto p : { r.getTopLeft() + juce::Point<float> (9, 9), r.getTopRight() + juce::Point<float> (-15, 9),
                    r.getBottomLeft() + juce::Point<float> (9, -15), r.getBottomRight() + juce::Point<float> (-15, -15) })
        g.fillEllipse (p.x, p.y, 6, 6);

    // nome do pedal
    const auto ink = body.getBrightness() > 0.6f ? juce::Colours::black : Colours::text;
    g.setColour (ink);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    auto titleArea = getLocalBounds().withTrimmedTop (getHeight() - 128).removeFromTop (28);
    g.drawText (title.toUpperCase(), titleArea, juce::Justification::centred);
}

void PedalComponent::resized()
{
    auto r = getLocalBounds().reduced (12);

    footswitch.setBounds (r.removeFromBottom (78).withSizeKeepingCentre (64, 78));
    r.removeFromBottom (40); // espaco do titulo

    for (auto it = extras.rbegin(); it != extras.rend(); ++it)
    {
        it->first->setBounds (r.removeFromBottom (it->second).reduced (4, 2));
        r.removeFromBottom (2);
    }

    if (knobs.empty())
        return;

    const int rows = ((int) knobs.size() + knobsPerRow - 1) / knobsPerRow;
    const int rowH = r.getHeight() / rows;

    for (int row = 0; row < rows; ++row)
    {
        auto rowArea = r.removeFromTop (rowH);
        const int first = row * knobsPerRow;
        const int count = juce::jmin (knobsPerRow, (int) knobs.size() - first);
        const int w = rowArea.getWidth() / count;
        for (int i = 0; i < count; ++i)
            knobs[(size_t) (first + i)]->setBounds (rowArea.removeFromLeft (w).reduced (2));
    }
}
