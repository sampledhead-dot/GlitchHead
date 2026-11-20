// Pruebas del motor en Node: sin NaN, nivel razonable, independiente del tamaño de bloque, rendimiento.
// node tests/test_dsp.js
const G = require('../dsp/glitchhead.js');
const sr = 48000;

// material de prueba: batería sintética + bajo + acorde (8 s, estéreo ligero)
function material(sec) {
  const n = Math.round(sec * sr), L = new Float32Array(n), R = new Float32Array(n);
  const rng = new G.Rng(777);
  const beat = 60 / 92;
  for (let i = 0; i < n; i++) {
    const t = i / sr, b = t / beat, ph = b % 1, step = Math.floor(b * 2) % 8, ph8 = (b * 2) % 1;
    let x = 0;
    if (ph < 0.25 && Math.floor(b) % 2 === 0) x += Math.sin(2 * Math.PI * (50 + 80 * Math.exp(-ph * 40)) * ph * beat) * Math.exp(-ph * 12) * 0.7; // bombo
    if (Math.floor(b) % 2 === 1 && ph < 0.2) x += (rng.next() * 2 - 1) * Math.exp(-ph * 25) * 0.4;                                          // caja
    x += (rng.next() * 2 - 1) * Math.exp(-ph8 * 60) * 0.12 * (step % 2 ? 0.6 : 1);                                                       // hats
    const f = [55, 55, 65.4, 49][Math.floor(t / (beat * 4)) % 4];
    x += Math.sin(2 * Math.PI * f * t) * 0.25 + Math.sin(2 * Math.PI * f * 2.997 * t) * 0.05;                                            // bajo
    const ch = 0.06 * (Math.sin(2 * Math.PI * 220 * t) + Math.sin(2 * Math.PI * 277.2 * t) + Math.sin(2 * Math.PI * 329.6 * t));
    L[i] = x + ch * 0.8; R[i] = x * 0.95 + ch;
  }
  return [L, R];
}
const rms = a => { let s = 0; for (const v of a) s += v * v; return Math.sqrt(s / a.length); };
const peak = a => { let m = 0; for (const v of a) m = Math.max(m, Math.abs(v)); return m; };
const db = x => (20 * Math.log10(x + 1e-12)).toFixed(1);

const [L, R] = material(8);
const rIn = rms(L);
let fail = 0;

for (const pr of G.PRESETS) {
  const params = Object.assign(G.defaults(), pr.v);
  const t0 = Date.now();
  const [oL, oR] = G.processBuffers([L, R], sr, params);
  const ms = Date.now() - t0;
  let nan = 0; for (let i = 0; i < oL.length; i++) if (!isFinite(oL[i]) || !isFinite(oR[i])) nan++;
  const rel = 20 * Math.log10(rms(oL) / rIn), pk = peak(oL);
  const cpu = (ms / 8000 * 100).toFixed(1);
  const bad = nan > 0 || pk > 4 || rel < -30 || rel > 12;
  if (bad) fail++;
  console.log(`${bad ? '✗' : '✓'} ${pr.cat.padEnd(7)} ${pr.name.padEnd(20)} nivel ${rel.toFixed(1).padStart(6)} dB · pico ${db(pk).padStart(6)} dBFS · ${cpu}% CPU (un núcleo) · NaN ${nan}`);
}

// independencia del tamaño de bloque (el plugin recibe bloques de cualquier tamaño)
function runBlocks(params, sizes) {
  const e = new G.Engine(sr); e.setParams(params);
  const oL = new Float32Array(L), oR = new Float32Array(R); let o = 0, k = 0;
  while (o < oL.length) { const m = Math.min(sizes[k++ % sizes.length], oL.length - o); e.process(oL.subarray(o, o + m), oR.subarray(o, o + m), m); o += m; }
  return oL;
}
const all = Object.assign(G.defaults(), { sOn: 1, hOn: 1, xOn: 1, sAbsence: -0.4, sDisorder: 0.5, xRetain: 0.5, hTemp: 0.8 });
const a = runBlocks(all, [512]), b = runBlocks(all, [1, 7, 64, 333, 1024, 13]);
let maxd = 0; for (let i = 0; i < a.length; i++) maxd = Math.max(maxd, Math.abs(a[i] - b[i]));
console.log(`${maxd < 1e-6 ? '✓' : '✗'} independiente del bloque: diferencia máx ${maxd.toExponential(2)}`);
if (maxd >= 1e-6) fail++;

// Shred con todo neutro (Dream 100 %, sin repetir ni desorden) reproduce la entrada con retardo: misma energía
const neutral = Object.assign(G.defaults(), { sDisorder: 0, sRepeat: 0, sLength: 4, sFloor: 0 });
const [nL] = G.processBuffers([L, R], sr, neutral);
const relN = 20 * Math.log10(rms(nL) / rIn);
console.log(`${Math.abs(relN) < 1.5 ? '✓' : '✗'} Shred neutro conserva el nivel: ${relN.toFixed(2)} dB`);
if (Math.abs(relN) >= 1.5) fail++;

// Sustain: tras cortar la entrada sigue sonando; sin Sustain se apaga
function tailLevel(sus) {
  const p = Object.assign(G.defaults(), { sSustain: sus, sRepeat: 0.6, sDisorder: 0.5, sLength: 16 });
  const x = material(4); for (let i = sr * 2; i < x[0].length; i++) { x[0][i] = 0; x[1][i] = 0; }
  const [o] = G.processBuffers(x, sr, p);
  return rms(o.subarray(Math.round(sr * 3.0)));
}
const s1 = tailLevel(1), s0 = tailLevel(0);
console.log(`${s1 > 0.01 && s0 < 1e-4 ? '✓' : '✗'} Sustain: con ${db(s1)} dB · sin ${db(s0)} dB`);
if (!(s1 > 0.01 && s0 < 1e-4)) fail++;

// Haze congelado mantiene la textura sin entrada
{
  const p = Object.assign(G.defaults(), { sOn: 0, hOn: 1, hWet: 1, hDry: 0, hSize: 500 });
  const e = new G.Engine(sr); e.setParams(p);
  const x = material(2); e.process(x[0], x[1], x[0].length);
  e.setParam('hFreeze', 1);
  const z = new Float32Array(sr), z2 = new Float32Array(sr); e.process(z, z2, sr);
  const ok = rms(z) > 0.01;
  console.log(`${ok ? '✓' : '✗'} Haze Freeze sostiene sin entrada: ${db(rms(z))} dB`);
  if (!ok) fail++;
}

// Split: Vol Below 0 y entrada por debajo del umbral → silencio
{
  const p = Object.assign(G.defaults(), { sOn: 0, xOn: 1, xVerge: 0, xVolB: 0 });
  const [o] = G.processBuffers([L, R], sr, p);
  const ok = rms(o) < 1e-3;
  console.log(`${ok ? '✓' : '✗'} Split: todo bajo el umbral y Below a 0 → ${db(rms(o))} dB`);
  if (!ok) fail++;
}

// el código del worklet se puede generar como texto
const src = G.workletSource();
console.log(`${src.length > 1000 && src.includes('registerProcessor') ? '✓' : '✗'} workletSource (${src.length} caracteres)`);

console.log(fail ? `FALLO (${fail})` : 'OK');
process.exit(fail ? 1 : 0);
