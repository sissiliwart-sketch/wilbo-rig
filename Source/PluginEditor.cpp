#include "PluginEditor.h"

using namespace ParamID;

//==============================================================================
void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 3.0f);

    const float db = juce::Decibels::gainToDecibels (level, -60.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    auto bar = r.reduced (2.0f);
    bar = bar.removeFromBottom (bar.getHeight() * norm);

    g.setColour (db > -3.0f ? Colours::ledOn : (db > -12.0f ? Colours::amber : Colours::good));
    g.fillRoundedRectangle (bar, 2.0f);
}

void SectionPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    juce::ColourGradient grad (juce::Colour (0xff2e2820), r.getX(), r.getY(),
                               juce::Colour (0xff1b1712), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (colour.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 8.0f, 1.2f);

    g.setColour (colour);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (title, r.reduced (14.0f, 8.0f).removeFromTop (20.0f), juce::Justification::centredLeft);
}

//==============================================================================
WilboRigEditor::WilboRigEditor (WilboRigProcessor& p)
    : AudioProcessorEditor (p), rig (p),
      inputKnob  (p.apvts, inGain,     "Input",  Colours::amber),
      gateKnob   (p.apvts, gateThresh, "Gate",   Colours::amber),
      outputKnob (p.apvts, outGain,    "Output", Colours::amber),
      gateAttachment  (p.apvts, gateOn, gateButton),
      tunerAttachment (p.apvts, tunerOn, tunerButton),
      tunerDisplay (p.getTuner()),
      wahPedal    (p.apvts, "Wah",       juce::Colour (0xff2b2b2b), wahOn,
                   { { ParamID::wahPedal, "Pedal" }, { wahQ, "Q" }, { wahSens, "Sens" } }, 3),
      drivePedal  (p.apvts, "Drive",     juce::Colour (0xffd8b43a), driveOn,
                   { { driveGain, "Drive" }, { driveTone, "Tone" }, { driveLevel, "Level" } }, 3),
      delayPedal  (p.apvts, "Delay",     juce::Colour (0xff2f6f8f), delayOn,
                   { { delayTime, "Time" }, { delayFb, "Repeat" }, { delayTone, "Tone" }, { delayMix, "Mix" } }, 2),
      reverbPedal (p.apvts, "Reverb",    juce::Colour (0xff6b4c8a), reverbOn,
                   { { reverbSize, "Size" }, { reverbDamp, "Damp" }, { reverbMix, "Mix" } }, 3),
      wahAutoAttachment (p.apvts, wahAuto, wahAutoButton),
      ampOnAttachment (p.apvts, ampOn, ampOnButton),
      cabOnAttachment (p.apvts, cabOn, cabOnButton)
{
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<juce::Component*> {
             &inputKnob, &gateKnob, &outputKnob, &gateButton, &tunerButton, &tunerDisplay,
             &inMeter, &outMeter, &wahPedal, &drivePedal, &delayPedal, &reverbPedal,
             &ampPanel, &cabPanel, &ampOnButton, &cabOnButton, &loadIrButton, &clearIrButton, &irLabel })
        addAndMakeVisible (c);

    tunerButton.setClickingTogglesState (true);
    tunerButton.setTooltip ("Liga o afinador e silencia a saida");

    wahPedal.addExtra (wahAutoButton);
    wahAutoButton.setTooltip ("Ligado: o wah abre conforme a forca da palhetada");

    driveTypeBox.addItemList ({ "Overdrive", "Distortion", "Fuzz" }, 1);
    driveTypeBox.setJustificationType (juce::Justification::centred);
    driveTypeAttachment = std::make_unique<ComboAttachment> (p.apvts, driveType, driveTypeBox);
    drivePedal.addExtra (driveTypeBox);

    const std::vector<std::pair<const char*, const char*>> ampParams {
        { ampGain, "Gain" }, { ampBass, "Bass" }, { ampMid, "Middle" },
        { ampTreble, "Treble" }, { ampPresence, "Presence" }, { ampMaster, "Master" } };

    for (auto& [id, name] : ampParams)
    {
        auto k = std::make_unique<Knob> (p.apvts, id, name, Colours::amber);
        addAndMakeVisible (*k);
        ampKnobs.push_back (std::move (k));
    }

    loadIrButton.onClick  = [this] { chooseImpulseResponse(); };
    clearIrButton.onClick = [this] { rig.getCabinet().clearImpulseResponse(); updateIrLabel(); };
    irLabel.setJustificationType (juce::Justification::centredLeft);
    irLabel.setFont (juce::FontOptions (12.0f));
    irLabel.setColour (juce::Label::textColourId, Colours::textDim);
    updateIrLabel();

    setSize (1180, 720);
    startTimerHz (30);
}

WilboRigEditor::~WilboRigEditor()
{
    setLookAndFeel (nullptr);
}

void WilboRigEditor::timerCallback()
{
    inMeter.setLevel (rig.getInputLevel());
    outMeter.setLevel (rig.getOutputLevel());
    tunerDisplay.setActive (tunerButton.getToggleState());
}

void WilboRigEditor::chooseImpulseResponse()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Escolha um arquivo IR de gabinete",
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                       "*.wav;*.aif;*.aiff");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto file = fc.getResult();
                                  if (file.existsAsFile())
                                      rig.getCabinet().loadImpulseResponse (file);
                                  updateIrLabel();
                              });
}

void WilboRigEditor::updateIrLabel()
{
    auto& cab = rig.getCabinet();
    irLabel.setText (cab.hasImpulseResponse() ? "IR: " + cab.getIrFile().getFileName()
                                              : "Simulador interno 4x12 (sem IR)",
                     juce::dontSendNotification);
}

void WilboRigEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);

    // faixa de "madeira" atras dos pedais
    auto board = getLocalBounds().withTrimmedTop (96).withHeight (372).toFloat().reduced (10.0f, 0.0f);
    g.setColour (Colours::wood);
    g.fillRoundedRectangle (board, 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    for (float y = board.getY() + 6.0f; y < board.getBottom(); y += 9.0f)
        g.drawHorizontalLine ((int) y, board.getX() + 6.0f, board.getRight() - 6.0f);

    // logo
    g.setColour (Colours::amber);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("WILBO RIG", 20, 18, 200, 30, juce::Justification::centredLeft);
    g.setColour (Colours::textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Wilbo Studio  |  pedaleira virtual", 20, 48, 240, 18, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("IN",  inMeter.getBounds().withY (inMeter.getBottom() + 2).withHeight (12).expanded (12, 0),  juce::Justification::centred);
    g.drawText ("OUT", outMeter.getBounds().withY (outMeter.getBottom() + 2).withHeight (12).expanded (12, 0), juce::Justification::centred);

    // cadeia de sinal
    g.setColour (Colours::textDim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Guitarra > Gate > Wah > Drive > Amp > Gabinete > Delay > Reverb > Saida",
                getLocalBounds().removeFromBottom (22), juce::Justification::centred);
}

void WilboRigEditor::resized()
{
    auto r = getLocalBounds();

    // --- Barra superior ---
    auto top = r.removeFromTop (92).reduced (12, 8);
    top.removeFromLeft (250); // logo

    inMeter.setBounds (top.removeFromLeft (12).withTrimmedBottom (16).withTrimmedTop (4));
    top.removeFromLeft (8);
    inputKnob.setBounds (top.removeFromLeft (70));
    top.removeFromLeft (8);
    auto gateArea = top.removeFromLeft (190);
    gateButton.setBounds (gateArea.removeFromLeft (115).withSizeKeepingCentre (115, 26));
    gateKnob.setBounds (gateArea);

    outMeter.setBounds (top.removeFromRight (12).withTrimmedBottom (16).withTrimmedTop (4));
    top.removeFromRight (8);
    outputKnob.setBounds (top.removeFromRight (70));
    top.removeFromRight (12);
    tunerButton.setBounds (top.removeFromRight (100).withSizeKeepingCentre (100, 34));
    top.removeFromRight (10);
    tunerDisplay.setBounds (top.reduced (10, 2));

    // --- Pedais ---
    auto board = r.removeFromTop (380).reduced (22, 14);
    const int pedalW = board.getWidth() / 4;
    for (auto* pedal : { &wahPedal, &drivePedal, &delayPedal, &reverbPedal })
        pedal->setBounds (board.removeFromLeft (pedalW).reduced (10, 0));

    // --- Amp + gabinete ---
    auto bottom = r.withTrimmedBottom (24).reduced (12, 4);
    cabPanel.setBounds (bottom.removeFromRight (340));
    bottom.removeFromRight (10);
    ampPanel.setBounds (bottom);

    auto ampArea = ampPanel.getContentArea().translated (ampPanel.getX(), ampPanel.getY());
    ampOnButton.setBounds (ampPanel.getRight() - 110, ampPanel.getY() + 10, 90, 24);
    const int kw = ampArea.getWidth() / (int) ampKnobs.size();
    for (auto& k : ampKnobs)
        k->setBounds (ampArea.removeFromLeft (kw).reduced (6, 4));

    auto cabArea = cabPanel.getContentArea().translated (cabPanel.getX(), cabPanel.getY());
    cabOnButton.setBounds (cabPanel.getRight() - 110, cabPanel.getY() + 10, 90, 24);
    cabArea.removeFromTop (10);
    loadIrButton.setBounds (cabArea.removeFromTop (34));
    cabArea.removeFromTop (8);
    clearIrButton.setBounds (cabArea.removeFromTop (30));
    cabArea.removeFromTop (6);
    irLabel.setBounds (cabArea.removeFromTop (40));
}
