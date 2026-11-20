// GENERADO por tests/gen_cpp.js a partir de PRESETS en dsp/glitchhead.js. No editar a mano.
#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

inline juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

struct FactoryPreset { const char* name; const char* cat; std::vector<std::pair<const char*, float>> values; };

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list {
        { "Default", "General", {  } },
        { "Rasguño leve", "Shred", { {"sLength",12.0f}, {"sDisorder",0.08f}, {"sRepeat",0.1f}, {"sDream",0.6f}, {"sSmoo",0.5f} } },
        { "Tartamudo", "Shred", { {"sLength",24.0f}, {"sFloor",3.0f}, {"sRepeat",0.7f}, {"sDisorder",0.1f} } },
        { "Cinta rota", "Shred", { {"sLength",16.0f}, {"sFloor",2.0f}, {"sStuck",0.4f}, {"sDisorder",0.5f}, {"sAbsence",0.35f}, {"sRate",-2.0f} } },
        { "Fuzz de wavesets", "Shred", { {"sLength",1.0f}, {"sFloor",0.0f}, {"sAbsence",0.55f}, {"sRepeat",0.3f}, {"sDisorder",0.0f} } },
        { "Sueño estéreo", "Shred", { {"sLength",20.0f}, {"sDream",0.5f}, {"sLean",0.3f}, {"sSmoo",0.8f}, {"sEnv",1.0f}, {"sDisorder",0.3f} } },
        { "Octava fantasma", "Shred", { {"sLength",6.0f}, {"sRate",12.0f}, {"sEnv",1.0f}, {"sRepeat",0.5f}, {"sMonitor",0.45f} } },
        { "Congelado", "Shred", { {"sLength",32.0f}, {"sFloor",3.0f}, {"sFreeze",1.0f}, {"sEnv",1.0f}, {"sSustain",1.0f} } },
        { "Halo", "Haze", { {"sOn",0.0f}, {"hOn",1.0f}, {"hMemory",0.55f}, {"hSize",450.0f}, {"hWet",0.45f}, {"hHi",3.0f}, {"hTemp",0.1f} } },
        { "Dron infinito", "Haze", { {"sOn",0.0f}, {"hOn",1.0f}, {"hMemory",0.92f}, {"hSize",1600.0f}, {"hWet",0.7f}, {"hDry",0.6f}, {"hLo",2.0f}, {"hHi",-4.0f} } },
        { "Metal que grita", "Haze", { {"sOn",0.0f}, {"hOn",1.0f}, {"hRaw",1.0f}, {"hMemory",0.6f}, {"hSize",60.0f}, {"hRate",7.0f}, {"hTemp",0.8f}, {"hWet",0.6f}, {"hHi",4.0f} } },
        { "Viento helado", "Haze", { {"sOn",0.0f}, {"hOn",1.0f}, {"hMemory",0.8f}, {"hSize",900.0f}, {"hRate",12.0f}, {"hLo",-10.0f}, {"hHi",6.0f}, {"hWidth",1.8f}, {"hWet",0.5f} } },
        { "Plomo fundido", "Haze", { {"sOn",0.0f}, {"hOn",1.0f}, {"hMemory",0.7f}, {"hSize",250.0f}, {"hRate",-12.0f}, {"hTemp",0.65f}, {"hLo",6.0f}, {"hHi",-6.0f}, {"hWet",0.6f} } },
        { "Puerta oscura", "Split", { {"sOn",0.0f}, {"xOn",1.0f}, {"xVerge",-24.0f}, {"xDriveA",0.5f}, {"xVolA",0.7f}, {"xVolB",0.0f} } },
        { "Caídas", "Split", { {"sOn",0.0f}, {"xOn",1.0f}, {"xVerge",-20.0f}, {"xRetain",0.7f}, {"xSmooth",8.0f}, {"xVolA",0.8f}, {"xVolB",0.2f} } },
        { "Textura temblorosa", "Split", { {"sOn",0.0f}, {"xOn",1.0f}, {"xVerge",-18.0f}, {"xVolA",0.3f}, {"xVolB",0.8f}, {"xDrift",0.7f}, {"xMelt",0.6f}, {"xSide",0.3f} } },
        { "Monstruo cruzado", "Split", { {"sOn",0.0f}, {"xOn",1.0f}, {"xMode",1.0f}, {"xVerge",-26.0f}, {"xBoost",6.0f}, {"xDriveA",0.8f}, {"xDriveB",0.6f}, {"xVolA",0.6f}, {"xVolB",0.5f} } },
        { "Bestia", "Combo", { {"sLength",4.0f}, {"sAbsence",0.3f}, {"sRepeat",0.4f}, {"xOn",1.0f}, {"xVerge",-22.0f}, {"xDriveA",0.7f}, {"xVolB",0.3f}, {"order",0.0f} } },
        { "Desintegrar loop", "Combo", { {"sLength",16.0f}, {"sFloor",2.0f}, {"sStuck",0.3f}, {"sDisorder",0.45f}, {"hOn",1.0f}, {"hMemory",0.6f}, {"hSize",700.0f}, {"hWet",0.35f} } },
        { "Sombra granular", "Combo", { {"sLength",10.0f}, {"sDream",0.7f}, {"sEnv",1.0f}, {"hOn",1.0f}, {"hMemory",0.75f}, {"hSize",1200.0f}, {"hWet",0.4f}, {"xOn",1.0f}, {"xVerge",-30.0f}, {"xVolA",0.6f}, {"xVolB",0.4f}, {"xMelt",0.5f}, {"order",2.0f} } },
    };
    return list;
}
