#include "PluginEditor.h"
#include "Presets.h"

static const juce::String kModNames[3] = { "SHRED", "HAZE", "SPLIT" };

static const ParamDef* findDef (const juce::String& id)
{
    for (const auto& d : kParamDefs) if (id == d.id) return &d;
    return nullptr;
}

//==============================================================================
GlitchLookAndFeel::GlitchLookAndFeel()
{
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2b2622));
    setColour (juce::ComboBox::textColourId, col::orangeL);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff2b2622));
    setColour (juce::ComboBox::arrowColourId, col::orangeL);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::headerTextColourId, col::orangeD);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, col::text);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2622));
    setColour (juce::TextButton::buttonOnColourId, col::accent);
    setColour (juce::TextButton::textColourOffId, col::cream);
    setColour (juce::TextButton::textColourOnId, col::text);
    setColour (juce::AlertWindow::backgroundColourId, col::panel);
    setColour (juce::AlertWindow::textColourId, col::text);
    setColour (juce::AlertWindow::outlineColourId, col::accent);
    setColour (juce::TextEditor::backgroundColourId, col::cream);
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::outlineColourId, col::text);
}

void GlitchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    // Botón redondo del logo: disco naranja, arco de valor, puntero crema
    const float r = (float) juce::jmin (w, h) * 0.5f - 3.0f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const bool bipolar = (bool) s.getProperties()["bipolar"];
    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    juce::Path bgArc; bgArc.addCentredArc (cx, cy, r, r, 0, a0, a1, true);
    g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (bgArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (std::abs (ang - from) > 0.01f)
    {
        juce::Path arc; arc.addCentredArc (cx, cy, r, r, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float kr = r * 0.66f;
    g.setColour (col::orangeD); g.fillEllipse (cx - kr, cy - kr + 2.0f, kr * 2, kr * 2);
    g.setColour (col::accent);  g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (col::orangeL); g.drawEllipse (cx - kr * 0.6f, cy - kr * 0.6f, kr * 1.2f, kr * 1.2f, 1.2f);
    juce::Path ptr; ptr.addRoundedRectangle (-1.5f, -kr + 2.0f, 3.0f, kr * 0.85f, 1.5f);
    g.setColour (col::cream); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
}

void GlitchLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre (40.0f, 22.0f);
    const bool on = b.getToggleState();
    const auto c = b.findColour (juce::ToggleButton::tickColourId);
    g.setColour (on ? c : col::panel2);
    g.fillRoundedRectangle (r, 11.0f);
    g.setColour (col::text); g.drawRoundedRectangle (r.reduced (1.0f), 10.0f, 2.0f);
    const float d = 14.0f, kx = on ? r.getRight() - d - 4.0f : r.getX() + 4.0f;
    g.fillEllipse (kx, r.getCentreY() - d / 2, d, d);
}

void GlitchLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    if (b.getProperties()["pad"])
    {
        const auto c = b.findColour (juce::TextButton::buttonOnColourId);
        g.setColour (on ? c : (over ? juce::Colour (0xfffbe3cf) : col::panel));
        g.fillRoundedRectangle (r, 4.0f);
        if (on) { g.setColour (c.darker (0.35f)); g.fillRect (r.removeFromBottom (3.0f).reduced (3.0f, 0.0f)); }
        else    { g.setColour (c); g.drawRoundedRectangle (r, 4.0f, 2.0f); }
        return;
    }
    auto c = on ? b.findColour (juce::TextButton::buttonOnColourId) : base;
    if (over && ! on) c = c.brighter (0.15f);
    if (down) c = c.darker (0.15f);
    if (! b.isEnabled()) c = c.withAlpha (0.35f);
    g.setColour (c); g.fillRoundedRectangle (r, 5.0f);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, bool onDark)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 14);
    slider.setColour (juce::Slider::rotarySliderFillColourId, onDark ? col::cream : col::text);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, onDark ? juce::Colour (0xff3d3631) : col::line);
    slider.setColour (juce::Slider::textBoxTextColourId, onDark ? col::cream.withAlpha (0.75f) : col::muted);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    if (auto* d = findDef (id)) slider.getProperties().set ("bipolar", d->min < 0 && d->max > 0);
    name.setText (label.toUpperCase(), juce::dontSendNotification);
    name.setJustificationType (juce::Justification::centred);
    name.setColour (juce::Label::textColourId, onDark ? col::cream : col::text);
    name.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    name.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (slider); addAndMakeVisible (name);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    getProperties().set ("pid", id);
}
void Knob::resized()
{
    auto r = getLocalBounds();
    name.setBounds (r.removeFromBottom (13));
    slider.setBounds (r);
}

Switch::Switch (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label)
{
    button.setColour (juce::ToggleButton::tickColourId, col::accent);
    name.setText (label.toUpperCase(), juce::dontSendNotification);
    name.setJustificationType (juce::Justification::centred);
    name.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    name.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (button); addAndMakeVisible (name);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
    getProperties().set ("pid", id);
}
void Switch::resized()
{
    auto r = getLocalBounds();
    name.setBounds (r.removeFromBottom (13));
    button.setBounds (r);
}

void ModulePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.withTrimmedTop (6.0f), 8.0f);
    auto plate = r.withTrimmedBottom (5.0f);
    g.setColour (col::panel); g.fillRoundedRectangle (plate, 8.0f);
    g.setColour (colour); g.fillRect (plate.getX() + 8.0f, 44.0f, plate.getWidth() - 16.0f, 3.0f);   // franja del motor
    g.setColour (col::orangeD.withAlpha (0.7f));                                                      // tornillos
    g.fillEllipse (8.0f, plate.getBottom() - 14.0f, 6.0f, 6.0f);
    g.fillEllipse (plate.getRight() - 14.0f, plate.getBottom() - 14.0f, 6.0f, 6.0f);
    // título como pegatina negra ligeramente girada
    const juce::Font f (juce::FontOptions (15.0f, juce::Font::bold));
    juce::GlyphArrangement ga; ga.addLineOfText (f, title, 0, 0);
    const float tw = ga.getBoundingBox (0, -1, true).getWidth();
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (-0.03f, 12.0f, 10.0f));
        g.setColour (col::text); g.fillRoundedRectangle (12.0f, 10.0f, tw + 18.0f, 25.0f, 3.0f);
        g.setColour (colour.brighter (0.4f)); g.fillRect (12.0f, 31.0f, tw + 18.0f, 4.0f);
        g.setColour (col::cream); g.setFont (f);
        g.drawText (title, 21, 10, (int) tw + 4, 24, juce::Justification::centredLeft);
    }
    g.setColour (col::muted); g.setFont (juce::FontOptions (11.0f, juce::Font::italic));
    g.drawText (subtitle, (int) tw + 38, 14, 140, 18, juce::Justification::centredLeft);
    if (showLeds)
        for (int k = 0; k < 2; ++k)
        {
            const float lx = plate.getRight() - 112.0f + k * 16.0f, ly = 18.0f, v = juce::jlimit (0.0f, 1.0f, led[k]);
            g.setColour (col::text); g.fillEllipse (lx - 1, ly - 1, 12, 12);
            g.setColour (col::led.withAlpha (0.15f + 0.85f * v)); g.fillEllipse (lx + 1, ly + 1, 8, 8);
        }
    g.setColour (col::muted); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    for (int i = 0; i < sectionLabels.size(); ++i)
    {
        const float yy = (float) sectionY[i];
        g.drawText (sectionLabels[i], 14, (int) yy, 80, 12, juce::Justification::centredLeft);
        g.setColour (col::line); g.fillRect (70.0f, yy + 6.0f, plate.getWidth() - 84.0f, 1.0f);
        g.setColour (col::muted);
    }
}

//==============================================================================
Knob* GlitchHeadEditor::knob (const juce::String& id, const juce::String& label, bool onDark)
{
    auto* k = knobs.add (new Knob (proc.apvts, id, label, onDark));
    addAndMakeVisible (k); byId[id] = k; return k;
}
Switch* GlitchHeadEditor::sw (const juce::String& id, const juce::String& label)
{
    auto* s = switches.add (new Switch (proc.apvts, id, label));
    addAndMakeVisible (s); byId[id] = s; return s;
}

GlitchHeadEditor::GlitchHeadEditor (GlitchHeadProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    for (auto* m : { &pShred, &pHaze, &pSplit }) addAndMakeVisible (*m);
    pShred.showLeds = pSplit.showLeds = true;

    // Barra superior: presets + globales
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId()); };
    for (auto* b : { &prevB, &nextB, &saveB, &delB, &lockB }) addAndMakeVisible (*b);
    prevB.onClick = [this] { int i = presetBox.getSelectedItemIndex(); presetBox.setSelectedItemIndex ((i - 1 + presetBox.getNumItems()) % presetBox.getNumItems()); };
    nextB.onClick = [this] { int i = presetBox.getSelectedItemIndex(); presetBox.setSelectedItemIndex ((i + 1) % presetBox.getNumItems()); };
    lockB.setClickingTogglesState (true);
    lockB.setToggleState (proc.lockGlobals.load(), juce::dontSendNotification);
    lockB.onClick = [this] { proc.lockGlobals.store (lockB.getToggleState()); };
    lockB.getProperties().set ("pid", "lock");
    saveB.onClick = [this]
    {
        auto* w = new juce::AlertWindow ("Guardar preset", "Nombre del preset:", juce::MessageBoxIconType::NoIcon, this);
        w->addTextEditor ("name", proc.currentPresetName == "Default" ? juce::String() : proc.currentPresetName);
        w->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (r != 1 || name.isEmpty()) return;
            if (name == "Default") { juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "GlitchHead", u8 ("\"Default\" está reservado.")); return; }
            auto doSave = [this, name] { proc.saveUserPreset (name); refreshPresets(); };
            if (proc.getUserPresetNames().contains (name))
                juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("GlitchHead")
                                              .withMessage ("Ya existe \"" + name + u8 ("\". ¿Sobrescribir?")).withButton (u8 ("Sí"), 1).withButton ("No", 0).withAssociatedComponent (this),
                                              [doSave] (int ok) { if (ok == 1) doSave(); });
            else doSave();
        }), true);
    };
    delB.onClick = [this]
    {
        const auto name = proc.currentPresetName;
        if (! proc.getUserPresetNames().contains (name)) return;
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("GlitchHead")
                                          .withMessage (u8 ("¿Borrar el preset \"") + name + "\"?").withButton ("Borrar", 1).withButton ("Cancelar", 0).withAssociatedComponent (this),
                                      [this, name] (int ok) { if (ok == 1) { proc.deleteUserPreset (name); proc.loadFactory (0); refreshPresets(); } });
    };
    knob ("inputDb", "Input", true); knob ("mix", "Mix", true); knob ("outputDb", "Output", true);

    // Interruptores On de cada motor y flechas para reordenar
    const char* onIds[3] = { "sOn", "hOn", "xOn" };
    const juce::Colour mc[3] = { col::shred, col::haze, col::split };
    for (int m = 0; m < 3; ++m)
    {
        onSwitch[m] = std::make_unique<juce::ToggleButton>();
        onSwitch[m]->setColour (juce::ToggleButton::tickColourId, mc[m]);
        onSwitch[m]->getProperties().set ("pid", onIds[m]);
        addAndMakeVisible (*onSwitch[m]);
        onAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, onIds[m], *onSwitch[m]));
        mvLeft[m].setButtonText ("<"); mvRight[m].setButtonText (">");
        mvLeft[m].onClick = [this, m] { moveModule (m, -1); };
        mvRight[m].onClick = [this, m] { moveModule (m, 1); };
        mvLeft[m].getProperties().set ("pid", "order"); mvRight[m].getProperties().set ("pid", "order");
        addAndMakeVisible (mvLeft[m]); addAndMakeVisible (mvRight[m]);
    }

    // SHRED
    const char* fl[4] = { "0", "25", "50", "100 ms" };
    for (int i = 0; i < 4; ++i)
    {
        auto* b = floorPads.add (new juce::TextButton (fl[i]));
        b->getProperties().set ("pad", true); b->getProperties().set ("pid", "sFloor");
        b->setColour (juce::TextButton::buttonOnColourId, col::shred);
        b->setColour (juce::TextButton::textColourOffId, col::text);
        b->onClick = [this, i] { setParam ("sFloor", (float) i); };
        addAndMakeVisible (b);
    }
    for (auto [id, lb] : { std::pair<const char*, const char*> { "sLength", "Length" }, { "sStuck", "Stuck" }, { "sDisorder", "Disorder" }, { "sRepeat", "Repeat" },
                           { "sRate", "Rate" }, { "sAbsence", "Absence" }, { "sDream", "Dream" }, { "sLean", "Lean" }, { "sSmoo", "Smoo" }, { "sMonitor", "Monitor" } })
        knob (id, lb);
    sw ("sEnv", "Envelope"); sw ("sFreeze", "Freeze"); sw ("sSync", "Sync"); sw ("sSustain", "Sustain");

    // HAZE
    for (auto [id, lb] : { std::pair<const char*, const char*> { "hMemory", "Memory" }, { "hSize", "Size" }, { "hRate", "Rate" }, { "hTemp", "Temp" },
                           { "hWidth", "Mid/Side" }, { "hLo", "Lo" }, { "hHi", "Hi" }, { "hWet", "Wet" }, { "hDry", "Dry" } })
        knob (id, lb);
    sw ("hRaw", "Raw"); sw ("hFreeze", "Freeze");

    // SPLIT
    const char* md[3] = { "Direct", "Crossed", "Summed" };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = modePads.add (new juce::TextButton (md[i]));
        b->getProperties().set ("pad", true); b->getProperties().set ("pid", "xMode");
        b->setColour (juce::TextButton::buttonOnColourId, col::split);
        b->setColour (juce::TextButton::textColourOffId, col::text);
        b->onClick = [this, i] { setParam ("xMode", (float) i); };
        addAndMakeVisible (b);
    }
    for (auto [id, lb] : { std::pair<const char*, const char*> { "xVerge", "Verge" }, { "xBoost", "Boost" }, { "xSmooth", "Smooth" }, { "xRetain", "Retain" },
                           { "xSide", "Sidechain" }, { "xDriveA", "Drive" }, { "xCutA", "Cut" }, { "xVolA", "Vol" },
                           { "xDrift", "Drift" }, { "xMelt", "Melt" }, { "xDriveB", "Drive" }, { "xCutB", "Cut" }, { "xVolB", "Vol" } })
        knob (id, lb);
    for (auto* s : switches) if (s->getProperties()["pid"].toString().startsWith ("h")) s->button.setColour (juce::ToggleButton::tickColourId, col::haze);

    versionLabel.setText (u8 ("GlitchHead · VST v") + GlitchHeadProcessor::kVersion + u8 (" · Sampled Head"), juce::dontSendNotification);
    versionLabel.setColour (juce::Label::textColourId, col::cream.withAlpha (0.8f));
    versionLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (versionLabel);
    infoLabel.setColour (juce::Label::textColourId, col::orangeL);
    infoLabel.setFont (juce::FontOptions (13.0f));
    infoLabel.setText (u8 ("Pasa el ratón por un mando para ver qué hace."), juce::dontSendNotification);
    addAndMakeVisible (infoLabel);
    addChildComponent (update);

    addMouseListener (this, true);
    refreshPresets();
    setSize (1124, 604);
    timerCallback();
    startTimerHz (20);
}

GlitchHeadEditor::~GlitchHeadEditor() { removeMouseListener (this); setLookAndFeel (nullptr); }

void GlitchHeadEditor::setParam (const juce::String& id, float v)
{
    if (auto* r = proc.apvts.getParameter (id))
    {
        r->beginChangeGesture();
        r->setValueNotifyingHost (r->convertTo0to1 (v));
        r->endChangeGesture();
    }
}

void GlitchHeadEditor::showInfo (const juce::String& id)
{
    if (id == hoverId) return;
    hoverId = id;
    if (id == "lock") { infoLabel.setText (u8 ("Candado: al cambiar de preset se conservan Input, Mix y Output."), juce::dontSendNotification); return; }
    if (auto* d = findDef (id)) infoLabel.setText (juce::String (d->name) + ": " + u8 (d->desc), juce::dontSendNotification);
}

void GlitchHeadEditor::mouseMove (const juce::MouseEvent& e)
{
    for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
        if (c->getProperties().contains ("pid")) { showInfo (c->getProperties()["pid"].toString()); return; }
}
void GlitchHeadEditor::mouseExit (const juce::MouseEvent&) {}

void GlitchHeadEditor::refreshPresets()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& f = factoryPresets();
    juce::String cat;
    for (int i = 0; i < (int) f.size(); ++i)
    {
        if (cat != f[(size_t) i].cat) { cat = f[(size_t) i].cat; presetBox.addSectionHeading (u8 ("Fábrica · ") + cat); }
        presetBox.addItem (u8 (f[(size_t) i].name), i + 1);
    }
    const auto user = proc.getUserPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSectionHeading ("Usuario");
        for (int i = 0; i < user.size(); ++i) presetBox.addItem (user[i], 1000 + i);
    }
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == proc.currentPresetName) { presetBox.setSelectedItemIndex (i, juce::dontSendNotification); break; }
    lastPresetName = proc.currentPresetName;
}

void GlitchHeadEditor::selectPreset (int itemId)
{
    if (itemId <= 0) return;
    if (itemId < 1000) proc.loadFactory (itemId - 1);
    else proc.loadUserPreset (presetBox.getItemText (presetBox.getSelectedItemIndex()));
    lastPresetName = proc.currentPresetName;
}

void GlitchHeadEditor::moveModule (int m, int dir)
{
    const int cur = juce::jlimit (0, 5, juce::roundToInt (getParam ("order")));
    int o[3] = { glitch::kOrders[cur][0], glitch::kOrders[cur][1], glitch::kOrders[cur][2] };
    int pos = 0; for (int k = 0; k < 3; ++k) if (o[k] == m) pos = k;
    const int np = pos + dir;
    if (np < 0 || np > 2) return;
    std::swap (o[pos], o[np]);
    for (int i = 0; i < 6; ++i)
        if (glitch::kOrders[i][0] == o[0] && glitch::kOrders[i][1] == o[1] && glitch::kOrders[i][2] == o[2]) { setParam ("order", (float) i); break; }
}

void GlitchHeadEditor::timerCallback()
{
    const int fl = juce::roundToInt (getParam ("sFloor"));
    for (int i = 0; i < 4; ++i) floorPads[i]->setToggleState (i == fl, juce::dontSendNotification);
    const int md = juce::roundToInt (getParam ("xMode"));
    for (int i = 0; i < 3; ++i) modePads[i]->setToggleState (i == md, juce::dontSendNotification);

    const float l0 = proc.leds[0].load(), l1 = proc.leds[1].load(), l2 = proc.leds[2].load(), l3 = proc.leds[3].load();
    if (std::abs (l0 - pShred.led[0]) > 0.02f || std::abs (l1 - pShred.led[1]) > 0.02f) { pShred.led[0] = l0; pShred.led[1] = l1; pShred.repaint(); }
    if (std::abs (l2 - pSplit.led[0]) > 0.02f || std::abs (l3 - pSplit.led[1]) > 0.02f) { pSplit.led[0] = l2; pSplit.led[1] = l3; pSplit.repaint(); }

    // atenuar los mandos de los motores apagados
    const bool on[3] = { getParam ("sOn") > 0.5f, getParam ("hOn") > 0.5f, getParam ("xOn") > 0.5f };
    for (auto& kv : byId)
    {
        const auto c0 = kv.first[0];
        const int m = c0 == 's' ? 0 : c0 == 'h' ? 1 : c0 == 'x' ? 2 : -1;
        if (m >= 0) kv.second->setAlpha (on[m] ? 1.0f : 0.4f);
    }
    for (auto* b : floorPads) b->setAlpha (on[0] ? 1.0f : 0.4f);
    for (auto* b : modePads) b->setAlpha (on[2] ? 1.0f : 0.4f);
    if (on[0]) byId["sRepeat"]->setAlpha (getParam ("sFreeze") > 0.5f ? 0.4f : 1.0f);

    const int order = juce::roundToInt (getParam ("order"));
    if (order != lastOrder) { lastOrder = order; resized(); }
    if (proc.currentPresetName != lastPresetName) refreshPresets();
}

void GlitchHeadEditor::paint (juce::Graphics& g)
{
    g.fillAll (col::bg);
    g.setColour (col::accent);
    g.fillEllipse (-40.0f, -210.0f, 380.0f, 330.0f);

    // icono: placa de sampler cortada en rodajas desplazadas (glitch) sobre el círculo naranja
    {
        const float cx = 46.0f, cy = 50.0f;
        g.setColour (col::orangeD); g.fillEllipse (cx - 28, cy - 26, 56, 56);
        g.setColour (col::accent);  g.fillEllipse (cx - 28, cy - 28, 56, 56);
        const float off[5] = { 0, 4, -3, 5, -1 };
        for (int k = 0; k < 5; ++k)
        {
            g.setColour (k == 3 ? col::text : col::cream);
            g.fillRect (cx - 13.0f + off[k], cy - 23.0f + k * 8.0f, 26.0f, 7.0f);
        }
        g.setColour (col::accent);
        for (int i = 0; i < 4; ++i) g.fillEllipse (cx - 10 + i * 6.0f, cy - 21, 3.5f, 3.5f);
        for (int c = 0; c < 4; ++c) g.fillRect (cx - 11 + c * 5.8f + off[2], cy - 6, 4.6f, 4.6f);
        for (int c = 0; c < 4; ++c) g.fillRect (cx - 11 + c * 5.8f + off[4], cy + 10, 4.6f, 4.6f);
    }
    // palabra GLITCHHEAD estilo grafiti con un corte desplazado
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (36.0f, juce::Font::bold | juce::Font::italic)), "GLITCHHEAD", 0, 0);
        juce::Path p; ga.createPath (p);
        p.applyTransform (juce::AffineTransform::rotation (-0.05f).translated (86.0f, 66.0f));
        g.setColour (juce::Colour (0xff2b2622)); g.fillPath (p, juce::AffineTransform::translation (4.0f, 4.0f));
        g.setColour (col::cream); g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col::text); g.fillPath (p);
        {   // rodaja desplazada
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (80, 44, 260, 8);
            g.setColour (col::cream); g.fillPath (p, juce::AffineTransform::translation (9.0f, 0.0f));
        }
        for (auto d : { std::pair<float,float> { 110.0f, 14.0f }, { 170.0f, 19.0f }, { 236.0f, 11.0f }, { 290.0f, 16.0f } })
        {
            const float yy = 66.0f - (d.first - 86.0f) * 0.05f;
            g.setColour (col::cream); g.fillRoundedRectangle (d.first - 3.0f, yy - 2.0f, 7.0f, d.second + 3.0f, 3.5f);
            g.setColour (col::text);  g.fillRoundedRectangle (d.first - 1.5f, yy - 2.0f, 4.0f, d.second, 2.0f);
        }
    }
    g.setColour (col::text); g.fillRoundedRectangle (86.0f, 92.0f, 210.0f, 18.0f, 2.0f);
    g.setColour (col::cream); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (u8 ("SHRED · HAZE · SPLIT"), 86, 92, 210, 18, juce::Justification::centred);

    // cadena
    const int order = juce::jlimit (0, 5, juce::roundToInt (getParam ("order")));
    juce::String chain = "Input > ";
    for (int k = 0; k < 3; ++k) chain << kModNames[glitch::kOrders[order][k]] << " > ";
    chain << "Mix > Output";
    g.setColour (col::cream.withAlpha (0.6f)); g.setFont (juce::FontOptions (12.0f));
    g.drawText (chain, 330, 98, 420, 16, juce::Justification::centredLeft);
}

void GlitchHeadEditor::resized()
{
    // barra superior
    presetBox.setBounds (330, 22, 250, 28);
    prevB.setBounds (586, 22, 30, 28); nextB.setBounds (620, 22, 30, 28);
    saveB.setBounds (330, 58, 76, 28); delB.setBounds (410, 58, 70, 28); lockB.setBounds (484, 58, 86, 28);
    byId["inputDb"]->setBounds (850, 8, 86, 100); byId["mix"]->setBounds (938, 8, 86, 100); byId["outputDb"]->setBounds (1026, 8, 86, 100);

    const int y0 = 124, w = 356, h = 416, gap = 12, cw = 68, kh = 92;
    const int order = juce::jlimit (0, 5, juce::roundToInt (getParam ("order")));
    ModulePanel* panels[3] = { &pShred, &pHaze, &pSplit };
    for (int slot = 0; slot < 3; ++slot)
    {
        const int m = glitch::kOrders[order][slot];
        const int x = 16 + slot * (w + gap);
        panels[m]->setBounds (x, y0, w, h);
        onSwitch[m]->setBounds (x + w - 56, y0 + 10, 46, 26);
        mvLeft[m].setBounds (x + w - 118, y0 + 12, 26, 22); mvRight[m].setBounds (x + w - 88, y0 + 12, 26, 22);
        mvLeft[m].setEnabled (slot > 0); mvRight[m].setEnabled (slot < 2);
        if (m == 0 || m == 2) { mvLeft[m].setBounds (x + w - 186, y0 + 12, 26, 22); mvRight[m].setBounds (x + w - 156, y0 + 12, 26, 22); }

        auto cell = [&] (int col, int row, int yb) { return juce::Rectangle<int> (x + 8 + col * cw, y0 + yb + row * (kh + 4), cw, kh); };
        auto place = [&] (const char* id, int c, int r, int yb) { byId[id]->setBounds (cell (c, r, yb)); };
        auto& P = *panels[m];
        P.sectionLabels.clear(); P.sectionY.clear();
        if (m == 0)
        {
            for (int i = 0; i < 4; ++i) floorPads[i]->setBounds (x + 70 + i * 64, y0 + 56, 60, 26);
            P.sectionLabels.add ("FLOOR"); P.sectionY.add (63);
            const char* r1[5] = { "sLength", "sStuck", "sDisorder", "sRepeat", "sRate" };
            const char* r2[5] = { "sAbsence", "sDream", "sLean", "sSmoo", "sMonitor" };
            for (int c = 0; c < 5; ++c) { place (r1[c], c, 0, 92); place (r2[c], c, 1, 92); }
            const char* s4[4] = { "sEnv", "sFreeze", "sSync", "sSustain" };
            for (int c = 0; c < 4; ++c) byId[s4[c]]->setBounds (x + 16 + c * 84, y0 + 300, 76, 44);
            P.sectionLabels.add ("MODO"); P.sectionY.add (288);
        }
        else if (m == 1)
        {
            byId["hRaw"]->setBounds (x + 16, y0 + 54, 76, 44); byId["hFreeze"]->setBounds (x + 100, y0 + 54, 76, 44);
            const char* r1[5] = { "hMemory", "hSize", "hRate", "hTemp", "hWidth" };
            for (int c = 0; c < 5; ++c) place (r1[c], c, 0, 108);
            P.sectionLabels.add ("EQ Y MEZCLA"); P.sectionY.add (204);
            const char* r2[4] = { "hLo", "hHi", "hWet", "hDry" };
            for (int c = 0; c < 4; ++c) place (r2[c], c, 0, 220);
        }
        else
        {
            for (int i = 0; i < 3; ++i) modePads[i]->setBounds (x + 70 + i * 86, y0 + 56, 82, 26);
            P.sectionLabels.add ("MODE"); P.sectionY.add (63);
            const char* r1[5] = { "xVerge", "xBoost", "xSmooth", "xRetain", "xSide" };
            for (int c = 0; c < 5; ++c) place (r1[c], c, 0, 92);
            P.sectionLabels.add ("ARRIBA"); P.sectionY.add (188);
            const char* r2[3] = { "xDriveA", "xCutA", "xVolA" };
            for (int c = 0; c < 3; ++c) place (r2[c], c, 0, 200);
            P.sectionLabels.add ("ABAJO"); P.sectionY.add (296);
            const char* r3[5] = { "xDrift", "xMelt", "xDriveB", "xCutB", "xVolB" };
            for (int c = 0; c < 5; ++c) place (r3[c], c, 0, 308);
        }
    }
    infoLabel.setBounds (16, y0 + h + 6, 1090, 20);
    versionLabel.setBounds (16, getHeight() - 30, 320, 20);
    update.setBounds (getWidth() - 496, getHeight() - 36, 480, 30);
    repaint();
}
