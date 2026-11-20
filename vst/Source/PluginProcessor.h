#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/GlitchDSP.h"
#include "ParamTable.h"

class GlitchHeadProcessor : public juce::AudioProcessor
{
public:
    static constexpr const char* kVersion = "1.0.0";   // igual que VERSION en CMakeLists.txt
    GlitchHeadProcessor();
    ~GlitchHeadProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void reset() override { if (engine) engine->reset(); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GlitchHead"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Presets de usuario (.preset = XML) en Documentos/GlitchHead/User Presets
    static juce::File userPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    void loadFactory (int index);
    juce::String currentPresetName { "Default" };

    // Candado: al cambiar de preset se conservan Input, Mix y Output
    std::atomic<bool> lockGlobals { false };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Indicadores para la ventana: [Shred L, Shred R, Split L, Split R] (0..1) y nivel de salida
    std::atomic<float> leds[4] { {0}, {0}, {0}, {0} };
    std::atomic<float> outDb { -60.0f };

    float p (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

private:
    std::unique_ptr<glitch::Engine> engine;
    std::array<std::atomic<float>*, kNumParams> raw {};
    std::array<float, kNumParams> last {};
    glitch::Params params;
    std::vector<float> tmpR;
    double sr = 44100.0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlitchHeadProcessor)
};
