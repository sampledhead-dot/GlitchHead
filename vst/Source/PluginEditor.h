#pragma once
#include "PluginProcessor.h"
#include "UpdateBanner.h"

namespace col
{
    // Paleta de Sampled Head: naranja del logo, placa crema de sampler y negro grafiti. Cada motor lleva un color de franja.
    const juce::Colour bg (0xff17130f), panel (0xfff3efe9), panel2 (0xffe8e2d9), line (0xffd6cec2),
                       text (0xff111111), muted (0xff6f675e), accent (0xffd9600b),
                       cream (0xfff3efe9), orangeD (0xffa94a07), orangeL (0xffef7a22),
                       shred (0xffd9600b), haze (0xff3f7f8c), split (0xff8c3f6e), led (0xffff4a2a);
}

class GlitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GlitchLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
};

// Knob con etiqueta, valor y descripción (se muestra abajo al pasar el ratón)
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, bool onDark = false);
    void resized() override;
    juce::Slider slider;
    juce::Label name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Interruptor con etiqueta
struct Switch : public juce::Component
{
    Switch (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label);
    void resized() override;
    juce::ToggleButton button;
    juce::Label name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

// Placa de un motor: fondo crema, franja de color, título como pegatina y pilotos L/R
struct ModulePanel : public juce::Component
{
    ModulePanel (const juce::String& t, const juce::String& sub, juce::Colour c) : title (t), subtitle (sub), colour (c) { setInterceptsMouseClicks (false, true); }
    void paint (juce::Graphics&) override;
    juce::String title, subtitle; juce::Colour colour; float led[2] { 0, 0 }; bool showLeds = false;
    juce::StringArray sectionLabels; juce::Array<int> sectionY;
};

class GlitchHeadEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GlitchHeadEditor (GlitchHeadProcessor&);
    ~GlitchHeadEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void setParam (const juce::String& id, float v);
    float getParam (const juce::String& id) const { return proc.p (id); }
    void refreshPresets();
    void selectPreset (int itemId);
    void moveModule (int m, int dir);
    Knob* knob (const juce::String& id, const juce::String& label, bool onDark = false);
    Switch* sw (const juce::String& id, const juce::String& label);
    void showInfo (const juce::String& id);

    GlitchHeadProcessor& proc;
    GlitchLookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton prevB { "<" }, nextB { ">" }, saveB { "Guardar" }, delB { "Borrar" }, lockB { "Candado" };
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Switch> switches;
    std::map<juce::String, juce::Component*> byId;

    ModulePanel pShred { "SHRED", "trocea en wavesets", col::shred }, pHaze { "HAZE", "memoria y granos", col::haze }, pSplit { "SPLIT", "puerta partida", col::split };
    std::unique_ptr<juce::ToggleButton> onSwitch[3];
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> onAtts;
    juce::TextButton mvLeft[3], mvRight[3];
    juce::OwnedArray<juce::TextButton> floorPads, modePads;

    juce::Label versionLabel, infoLabel;
public:
    shupdate::UpdateBanner update { "GlitchHead", "GlitchHead", GlitchHeadProcessor::kVersion, col::text, col::cream, col::accent };
private:
    int lastOrder = -1;
    juce::String lastPresetName, hoverId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchHeadEditor)
};
