// Paridad JS ↔ C++ con todos los presets de fábrica y una combinación extrema.
// g++ -O2 -std=c++17 tests/parity.cpp -o /tmp/gh_parity && node tests/parity.js /tmp/gh_parity
const G = require('../dsp/glitchhead.js');
const fs = require('fs'), os = require('os'), path = require('path'), { execFileSync } = require('child_process');
const bin = process.argv[2] || '/tmp/gh_parity';
const sr = 48000, n = sr * 4;
const rng = new G.Rng(99), L = new Float32Array(n), R = new Float32Array(n);
for (let i = 0; i < n; i++) {
  const t = i / sr, ph = (t * 2.2) % 1;
  const x = Math.sin(2 * Math.PI * 82.4 * t) * 0.3 * Math.exp(-ph * 3) + (rng.next() * 2 - 1) * 0.2 * Math.exp(-ph * 30)
          + Math.sin(2 * Math.PI * 330 * t + Math.sin(2 * Math.PI * 0.5 * t)) * 0.15;
  L[i] = x; R[i] = x * 0.8 + Math.sin(2 * Math.PI * 247 * t) * 0.1;
}
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'ghp-'));
const inter = new Float32Array(2 * n); for (let i = 0; i < n; i++) { inter[2 * i] = L[i]; inter[2 * i + 1] = R[i]; }
fs.writeFileSync(path.join(dir, 'in.f32'), Buffer.from(inter.buffer));
const cases = G.PRESETS.map(p => [p.name, p.v]);
cases.push(['Todo a la vez', { sOn: 1, hOn: 1, xOn: 1, sAbsence: -0.5, sDisorder: 0.6, sStuck: 0.3, sRate: 5, sEnv: 1, sDream: 0.5, sLean: -0.4,
  hRaw: 1, hTemp: 0.9, hRate: -7, hWidth: 1.7, xMode: 2, xRetain: 0.8, xCutA: 0.5, xCutB: 0.4, xSide: 0.5, xDrift: 0.9, xMelt: 0.8, order: 3 }]);
cases.push(['Sync + Freeze', { sSync: 1, sFreeze: 1, sLength: 3, sFloor: 0, hOn: 1, hFreeze: 1, order: 5 }]);
let worst = 0, fail = 0;
for (const [name, v] of cases) {
  const params = Object.assign(G.defaults(), v);
  fs.writeFileSync(path.join(dir, 'p.txt'), Object.entries(params).map(([k, x]) => k + ' ' + x).join('\n'));
  const B = [64, 333, 512][cases.indexOf(cases.find(c => c[0] === name)) % 3];
  execFileSync(bin, [String(sr), path.join(dir, 'in.f32'), path.join(dir, 'p.txt'), path.join(dir, 'out.f32'), String(B)]);
  const c = new Float32Array(fs.readFileSync(path.join(dir, 'out.f32')).buffer.slice(0));
  const [jL, jR] = G.processBuffers([L, R], sr, params);
  let md = 0, at = -1;
  for (let i = 0; i < n; i++) { const d = Math.max(Math.abs(jL[i] - c[2 * i]), Math.abs(jR[i] - c[2 * i + 1])); if (d > md) { md = d; at = i; } }
  worst = Math.max(worst, md);
  const ok = md < 1e-4; if (!ok) fail++;
  console.log(`${ok ? '✓' : '✗'} ${name.padEnd(20)} bloque ${String(B).padStart(3)} · diferencia máx ${md.toExponential(2)}${ok ? '' : ' en la muestra ' + at}`);
}
console.log(fail ? `FALLO (${fail})` : `OK · peor diferencia ${worst.toExponential(2)}`);
process.exit(fail ? 1 : 0);
