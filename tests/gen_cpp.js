// Genera vst/Source/ParamTable.h y vst/Source/Presets.h a partir de PARAMS y PRESETS de dsp/glitchhead.js
// node tests/gen_cpp.js
const G = require('../dsp/glitchhead.js');
const fs = require('fs'), path = require('path');
const esc = s => JSON.stringify(s);
const F = v => (Number.isInteger(v) ? v + '.0' : String(v)) + 'f';
let t = '// GENERADO por tests/gen_cpp.js a partir de dsp/glitchhead.js. No editar a mano.\n#pragma once\n\n';
t += 'struct ParamDef { const char* id; const char* name; float min, max, def; char type; const char* choices; const char* desc; };\n\n';
t += 'static const ParamDef kParamDefs[] = {\n';
for (const [id, name, mn, mx, df, ty, desc] of G.PARAMS) {
  const type = ty[0], ch = ty.startsWith('c:') ? ty.slice(2) : '';
  t += `    { ${esc(id)}, ${esc(name)}, ${F(mn)}, ${F(mx)}, ${F(df)}, '${type}', ${esc(ch)}, ${esc(desc)} },\n`;
}
t += '};\nstatic constexpr int kNumParams = ' + G.PARAMS.length + ';\n';
fs.writeFileSync(path.join(__dirname, '../vst/Source/ParamTable.h'), t);

let p = '// GENERADO por tests/gen_cpp.js a partir de PRESETS en dsp/glitchhead.js. No editar a mano.\n#pragma once\n#include <juce_core/juce_core.h>\n#include <vector>\n#include <utility>\n\n';
p += 'inline juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }\n\n';
p += 'struct FactoryPreset { const char* name; const char* cat; std::vector<std::pair<const char*, float>> values; };\n\n';
p += 'inline const std::vector<FactoryPreset>& factoryPresets()\n{\n    static const std::vector<FactoryPreset> list {\n';
for (const pr of G.PRESETS) {
  const vals = Object.entries(pr.v).map(([k, v]) => `{${esc(k)},${F(v)}}`).join(', ');
  p += `        { ${esc(pr.name)}, ${esc(pr.cat)}, { ${vals} } },\n`;
}
p += '    };\n    return list;\n}\n';
fs.writeFileSync(path.join(__dirname, '../vst/Source/Presets.h'), p);
console.log('ok: ' + G.PARAMS.length + ' parámetros, ' + G.PRESETS.length + ' presets');
