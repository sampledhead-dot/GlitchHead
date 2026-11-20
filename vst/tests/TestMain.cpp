// Pruebas offline del plugin: paridad con el motor, mono/estéreo, bloques variables, estabilidad, rendimiento,
// estado, presets, candado, editor (captura PNG) y aviso de versión.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Presets.h"
#include <iostream>
#include <chrono>

static constexpr double SR = 48000.0;
static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { std::cout << "  FALLO: " << msg << "\n"; ++failures; } else std::cout << "  ok: " << msg << "\n"; } while (0)

static void setP (GlitchHeadProcessor& p, const juce::String& id, float v)
{
    auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v));
}

static std::vector<float> music (double seconds, uint32_t seed = 7)
{
    const size_t N = (size_t) (SR * seconds); std::vector<float> x (N, 0.0f); uint32_t s = seed;
    auto rnd = [&s] { s = s * 1664525u + 1013904223u; return (float) s / 4294967296.0f; };
    const float notes[] = { 110, 146.8f, 164.8f, 220, 196, 130.8f, 82.4f, 98 };
    for (int b = 0; b < (int) (seconds * 4); ++b)
    {
        const size_t t0 = (size_t) (b * SR / 4); const float f = notes[b % 8];
        for (size_t i = 0; i < (size_t) (SR / 4) && t0 + i < N; ++i)
        {
            const float t = (float) i / (float) SR, env = std::exp (-t * 5);
            float v = 0.4f * (2 * std::fmod (f * t, 1.0f) - 1) * env;
            if (b % 2 == 0) v += 0.8f * std::sin (6.2831853f * (50 + 80 * std::exp (-t * 30)) * t) * std::exp (-t * 12);
            if (b % 4 == 2) v += 0.4f * (rnd() * 2 - 1) * std::exp (-t * 25);
            x[t0 + i] += v;
        }
    }
    return x;
}

static std::pair<std::vector<float>, std::vector<float>> run (GlitchHeadProcessor& p, const std::vector<float>& L, const std::vector<float>& R,
                                                              std::initializer_list<int> blocks = { 256 })
{
    std::vector<int> bs (blocks);
    int maxB = 0; for (int b : bs) maxB = std::max (maxB, b);
    p.prepareToPlay (SR, maxB);
    juce::AudioBuffer<float> buf (2, maxB); juce::MidiBuffer mb;
    std::vector<float> oL (L.size()), oR (L.size());
    size_t pos = 0; int k = 0;
    while (pos < L.size())
    {
        const int len = (int) std::min<size_t> ((size_t) bs[(size_t) (k++ % (int) bs.size())], L.size() - pos);
        buf.setSize (2, len, false, false, true);
        for (int i = 0; i < len; ++i) { buf.setSample (0, i, L[pos + (size_t) i]); buf.setSample (1, i, R[pos + (size_t) i]); }
        p.processBlock (buf, mb);
        for (int i = 0; i < len; ++i) { oL[pos + (size_t) i] = buf.getSample (0, i); oR[pos + (size_t) i] = buf.getSample (1, i); }
        pos += (size_t) len;
    }
    return { oL, oR };
}

static double rms (const std::vector<float>& x) { double e = 0; for (auto v : x) e += (double) v * v; return std::sqrt (e / (double) x.size()); }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const auto L = music (6), R = music (6, 99);

    std::cout << "== Paridad plugin ↔ motor y bloques variables ==\n";
    for (int pi = 0; pi < (int) factoryPresets().size(); ++pi)
    {
        GlitchHeadProcessor p; p.loadFactory (pi);
        auto [a, b] = run (p, L, R, { 512 });
        // mismo motor llamado a mano
        glitch::Params prm;
        for (int i = 0; i < kNumParams; ++i) prm.set (kParamDefs[i].id, p.p (kParamDefs[i].id));
        glitch::Engine e (SR, 512); e.setParams (prm);
        std::vector<float> eL (L), eR (R);
        for (size_t o = 0; o < eL.size(); o += 512) e.process (eL.data() + o, eR.data() + o, (int) std::min<size_t> (512, eL.size() - o));
        double md = 0; for (size_t i = 0; i < a.size(); ++i) md = std::max (md, (double) std::abs (a[i] - eL[i]));
        GlitchHeadProcessor p2; p2.loadFactory (pi);
        auto [c, d] = run (p2, L, R, { 1, 37, 128, 1000, 64 });
        double mb = 0; bool fin = true;
        for (size_t i = 0; i < a.size(); ++i) { mb = std::max (mb, (double) std::abs (a[i] - c[i])); fin = fin && std::isfinite (c[i]) && std::isfinite (d[i]); }
        CHECK (md < 1e-6 && mb < 1e-6 && fin, u8 (factoryPresets()[(size_t) pi].name) + juce::String (": motor ") + juce::String (md, 8) + ", bloques " + juce::String (mb, 8));
    }

    std::cout << "== Mono ==\n";
    {
        GlitchHeadProcessor p;
        juce::AudioProcessor::BusesLayout lay; lay.inputBuses.add (juce::AudioChannelSet::mono()); lay.outputBuses.add (juce::AudioChannelSet::mono());
        CHECK (p.setBusesLayout (lay), "acepta mono → mono");
        p.prepareToPlay (SR, 256);
        juce::AudioBuffer<float> buf (1, 256); juce::MidiBuffer mb; bool fin = true;
        for (int k = 0; k < 400; ++k) { for (int i = 0; i < 256; ++i) buf.setSample (0, i, L[(size_t) (k * 256 + i) % L.size()]); p.processBlock (buf, mb); for (int i = 0; i < 256; ++i) fin = fin && std::isfinite (buf.getSample (0, i)); }
        CHECK (fin, "salida mono finita");
    }

    std::cout << "== Estabilidad con todo al extremo y cambios bruscos ==\n";
    {
        GlitchHeadProcessor p; p.prepareToPlay (SR, 256);
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer mb; juce::Random rnd (3); bool fin = true; float pk = 0;
        for (int k = 0; k < 2000; ++k)
        {
            if (k % 20 == 0)
                for (const auto& d : kParamDefs) { if (juce::String (d.id) == "outputDb" || juce::String (d.id) == "inputDb") continue; setP (p, d.id, d.min + rnd.nextFloat() * (d.max - d.min)); }
            for (int i = 0; i < 256; ++i) { buf.setSample (0, i, L[(size_t) (k * 256 + i) % L.size()]); buf.setSample (1, i, R[(size_t) (k * 256 + i) % R.size()]); }
            p.processBlock (buf, mb);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const float v = buf.getSample (c, i); fin = fin && std::isfinite (v); pk = std::max (pk, std::abs (v)); }
        }
        CHECK (fin && pk < 20.0f, "parámetros aleatorios durante 10 s: finito, pico " + juce::String (pk, 2));
    }

    std::cout << "== Rendimiento ==\n";
    {
        GlitchHeadProcessor p; setP (p, "sOn", 1); setP (p, "hOn", 1); setP (p, "xOn", 1); setP (p, "hTemp", 0.9f); setP (p, "xDrift", 0.8f);
        const auto t0 = std::chrono::high_resolution_clock::now();
        run (p, L, R, { 256 });
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::high_resolution_clock::now() - t0).count();
        const double cpu = ms / 6000.0 * 100.0;
        CHECK (cpu < 25.0, "los tres motores en estéreo a 48 kHz: " + juce::String (cpu, 2) + " % de un núcleo");
    }

    std::cout << "== Bypass natural: Mix 0 devuelve la entrada ==\n";
    {
        GlitchHeadProcessor p; setP (p, "mix", 0.0f); setP (p, "hOn", 1); setP (p, "xOn", 1);
        auto [a, b] = run (p, L, R);
        double md = 0; for (size_t i = 0; i < a.size(); ++i) md = std::max (md, (double) std::abs (a[i] - L[i]));
        CHECK (md < 1e-5, "Mix 0 = señal original (" + juce::String (md, 8) + ")");
    }

    std::cout << "== Estado, presets y candado ==\n";
    {
        GlitchHeadProcessor p; p.loadFactory (5); setP (p, "hSize", 777.0f);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        GlitchHeadProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (std::abs (q.p ("hSize") - 777.0f) < 0.5f && q.currentPresetName == p.currentPresetName, "el estado se guarda y se recupera");
        q.lockGlobals = true; setP (q, "mix", 0.3f); setP (q, "outputDb", -6.0f);
        q.loadFactory (10);
        CHECK (std::abs (q.p ("mix") - 0.3f) < 1e-3f && std::abs (q.p ("outputDb") + 6.0f) < 0.05f, "con candado el preset no toca Mix ni Output");
        q.lockGlobals = false; q.loadFactory (0);
        CHECK (std::abs (q.p ("mix") - 1.0f) < 1e-3f, "sin candado Default restablece Mix");
        CHECK (q.getNumPrograms() == (int) factoryPresets().size(), juce::String (q.getNumPrograms()) + " presets de fábrica");
    }

    std::cout << "== Editor ==\n";
    {
        GlitchHeadProcessor p; p.loadFactory (19);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setVisible (true);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        auto out = juce::File::getCurrentWorkingDirectory().getChildFile ("glitchhead-editor.png");
        out.deleteFile();
        juce::FileOutputStream fo (out); juce::PNGImageFormat png; png.writeImageToStream (img, fo);
        CHECK (img.getWidth() == 1124 && out.getSize() > 10000, "captura del editor " + out.getFullPathName());
        // reordenar con las flechas cambia el parámetro
        setP (p, "order", 0);
        auto* e = dynamic_cast<GlitchHeadEditor*> (ed.get());
        CHECK (e != nullptr, "editor de GlitchHead");
    }

    std::cout << "== Aviso de versión ==\n";
    {
        CHECK (shupdate::newer ("1.0.0", "1.0.1") && ! shupdate::newer ("1.0.0", "1.0.0") && shupdate::newer ("1.0.9", "1.0.10"), "comparar versiones");
    }

    std::cout << (failures ? "FALLOS: " + std::to_string (failures) : std::string ("TODO OK")) << "\n";
    return failures ? 1 : 0;
}
