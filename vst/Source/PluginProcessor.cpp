#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

juce::AudioProcessorValueTreeState::ParameterLayout GlitchHeadProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto P = [] (const char* id) { return juce::ParameterID { id, 1 }; };
    for (const auto& d : kParamDefs)
    {
        const juce::String id (d.id), name (d.name);
        if (d.type == 'b')
            v.push_back (std::make_unique<juce::AudioParameterBool> (P (d.id), name, d.def > 0.5f));
        else if (d.type == 'c')
            v.push_back (std::make_unique<juce::AudioParameterChoice> (P (d.id), name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) d.def));
        else if (d.type == 'i')
            v.push_back (std::make_unique<juce::AudioParameterInt> (P (d.id), name, (int) d.min, (int) d.max, (int) d.def));
        else
        {
            juce::NormalisableRange<float> r (d.min, d.max);
            juce::AudioParameterFloatAttributes a;
            if (id == "hSize" || id == "xSmooth")
            {
                r = juce::NormalisableRange<float> (d.min, d.max,
                        [] (float lo, float hi, float t) { return lo * std::pow (hi / lo, t); },
                        [] (float lo, float hi, float x) { return std::log (x / lo) / std::log (hi / lo); });
                a = a.withStringFromValueFunction ([] (float x, int) { return x >= 1000 ? juce::String (x / 1000.0f, 2) + " s" : juce::String (juce::roundToInt (x)) + " ms"; });
            }
            else if (id.endsWith ("Db") || id == "hLo" || id == "hHi" || id == "xVerge" || id == "xBoost")
                a = a.withStringFromValueFunction ([] (float x, int) { return (x > 0.04f ? "+" : "") + juce::String (x, 1) + " dB"; });
            else if (id == "sRate" || id == "hRate")
                a = a.withStringFromValueFunction ([] (float x, int) { return (x > 0.04f ? "+" : "") + juce::String (x, 1) + " st"; });
            else if (id == "sAbsence" || id == "sLean")
                a = a.withStringFromValueFunction ([] (float x, int) { return (x > 0.004f ? "+" : "") + juce::String (juce::roundToInt (x * 100)) + " %"; });
            else if (id == "hWidth")
                a = a.withStringFromValueFunction ([] (float x, int) { return juce::String (juce::roundToInt (x * 100)) + " %"; });
            else
                a = a.withStringFromValueFunction ([] (float x, int) { return juce::String (juce::roundToInt (x * 100)) + " %"; });
            v.push_back (std::make_unique<juce::AudioParameterFloat> (P (d.id), name, r, d.def, a));
        }
    }
    return { v.begin(), v.end() };
}

GlitchHeadProcessor::GlitchHeadProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < kNumParams; ++i) raw[(size_t) i] = apvts.getRawParameterValue (kParamDefs[i].id);
}

bool GlitchHeadProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    const bool inOk  = in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
    const bool outOk = out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return inOk && outOk;
}

void GlitchHeadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // el motor reserva aquí toda su memoria (búferes de ~5 s por canal y motor)
    if (engine == nullptr || std::abs (sampleRate - sr) > 0.5)
        engine = std::make_unique<glitch::Engine> (sampleRate, juce::jmax (512, samplesPerBlock));
    else engine->reset();
    sr = sampleRate;
    tmpR.assign ((size_t) juce::jmax (512, samplesPerBlock), 0.0f);
    for (auto& x : last) x = std::numeric_limits<float>::quiet_NaN();   // fuerza leer todos los parámetros
}

void GlitchHeadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if (engine == nullptr || n == 0) return;

    // Parámetros: solo se pasan al motor si alguno cambió
    bool changed = false;
    for (int i = 0; i < kNumParams; ++i)
    {
        const float x = raw[(size_t) i]->load();
        if (! (x == last[(size_t) i])) { last[(size_t) i] = x; params.set (kParamDefs[i].id, (double) x); changed = true; }
    }
    if (changed) engine->setParams (params);

    float* L = buffer.getWritePointer (0);
    float* R;
    if (outCh > 1) { R = buffer.getWritePointer (1); if (inCh < 2) std::copy (L, L + n, R); }
    else
    {
        if ((int) tmpR.size() < n) return;   // no debería pasar: el host promete samplesPerBlock
        std::copy (L, L + n, tmpR.begin()); R = tmpR.data();
    }
    engine->process (L, R, n);
    if (outCh == 1) for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int c = 2; c < outCh; ++c) buffer.clear (c, 0, n);

    double l4[4]; engine->leds (l4);
    for (int k = 0; k < 4; ++k) leds[k].store ((float) l4[k]);
    double e = 0; for (int i = 0; i < n; ++i) e += (double) L[i] * L[i];
    outDb.store (juce::Decibels::gainToDecibels ((float) std::sqrt (e / n), -60.0f));
}

//==============================================================================
int GlitchHeadProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String GlitchHeadProcessor::getProgramName (int i)
{
    const auto& f = factoryPresets();
    return juce::isPositiveAndBelow (i, (int) f.size()) ? u8 (f[(size_t) i].name) : juce::String();
}
void GlitchHeadProcessor::setCurrentProgram (int i) { loadFactory (i); }

void GlitchHeadProcessor::loadFactory (int index)
{
    const auto& f = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) f.size())) return;
    const bool lock = lockGlobals.load();
    for (const auto& d : kParamDefs)
    {
        const juce::String id (d.id);
        if (lock && (id == "inputDb" || id == "mix" || id == "outputDb")) continue;
        if (auto* r = apvts.getParameter (id)) r->setValueNotifyingHost (r->getDefaultValue());
    }
    for (auto& kv : f[(size_t) index].values)
        if (auto* r = apvts.getParameter (kv.first)) r->setValueNotifyingHost (r->convertTo0to1 (kv.second));
    currentProgram = index;
    currentPresetName = u8 (f[(size_t) index].name);
}

juce::File GlitchHeadProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
             .getChildFile ("GlitchHead").getChildFile ("User Presets");
}
juce::StringArray GlitchHeadProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.preset"))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}
bool GlitchHeadProcessor::saveUserPreset (const juce::String& name)
{
    if (name.trim().isEmpty() || name == "Default") return false;
    auto dir = userPresetFolder(); dir.createDirectory();
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr || ! xml->writeTo (dir.getChildFile (juce::File::createLegalFileName (name) + ".preset"))) return false;
    currentPresetName = name;
    return true;
}
bool GlitchHeadProcessor::loadUserPreset (const juce::String& name)
{
    auto xml = juce::XmlDocument::parse (userPresetFolder().getChildFile (name + ".preset"));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    if (lockGlobals.load())
    {
        for (auto* id : { "inputDb", "mix", "outputDb" })
            for (auto* c : xml->getChildIterator())
                if (c->getStringAttribute ("id") == id) c->setAttribute ("value", (double) p (id));
    }
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentPresetName = name;
    return true;
}
bool GlitchHeadProcessor::deleteUserPreset (const juce::String& name)
{
    return userPresetFolder().getChildFile (name + ".preset").deleteFile();
}

void GlitchHeadProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("lock", lockGlobals.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}
void GlitchHeadProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPresetName = vt.getProperty ("presetName", "Default").toString();
            lockGlobals.store ((bool) vt.getProperty ("lock", false));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* GlitchHeadProcessor::createEditor() { return new GlitchHeadEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GlitchHeadProcessor(); }
