// GlitchHead — motor DSP en C++ (sin JUCE). Mismo diseño que dsp/glitchhead.js.
// PARIDAD: cualquier cambio de sonido en los dos archivos (node tests/parity.js lo comprueba).
// Sin reservas de memoria en process(): todo se reserva en el constructor / prepare().
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <string>
#include <map>

namespace glitch
{
static constexpr double kPi = 3.14159265358979323846;
static constexpr double kTau = 2.0 * kPi;
static constexpr int kBufBits = 18, kBufN = 1 << kBufBits, kBufMask = kBufN - 1;
static constexpr int kOrders[6][3] = { {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
static constexpr double kFloorMs[4] = { 0, 25, 50, 100 };

inline double dbToGain (double d) { return std::pow (10.0, d / 20.0); }
inline double clampd (double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
inline double jsRound (double x) { return std::floor (x + 0.5); }   // Math.round de JS

struct Rng   // xorshift32 → [0,1)
{
    uint32_t s = 1;
    explicit Rng (uint32_t seed = 1) : s (seed ? seed : 1u) {}
    double next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (double) s / 4294967296.0; }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void clear() { x1 = x2 = y1 = y2 = 0; }
    void shelf (double sr, double f, double db, bool high)
    {
        const double A = std::pow (10.0, db / 40.0), w = kTau * f / sr, cs = std::cos (w), sn = std::sin (w);
        const double al = sn / 2.0 * std::sqrt (2.0), sA = 2.0 * std::sqrt (A) * al;
        double B0, B1, B2, A0, A1, A2;
        if (! high)
        {
            B0 = A * ((A + 1) - (A - 1) * cs + sA); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sA);
            A0 = (A + 1) + (A - 1) * cs + sA; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sA;
        }
        else
        {
            B0 = A * ((A + 1) + (A - 1) * cs + sA); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - sA);
            A0 = (A + 1) - (A - 1) * cs + sA; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - sA;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    double run (double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y; return y;
    }
};

struct Svf
{
    double g = 0, k = std::sqrt (2.0), a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0, lp = 0, hp = 0;
    void clear() { ic1 = ic2 = 0; }
    void set (double sr, double f, double q)
    {
        f = clampd (f, 10.0, sr * 0.45); g = std::tan (kPi * f / sr); k = 1.0 / q;
        a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    double run (double v0)
    {
        const double v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        lp = v2; hp = v0 - k * v1 - v2; return v2;
    }
};

// Parámetros (mismos ids y defectos que PARAMS en glitchhead.js)
struct Params
{
    double inputDb = 0, mix = 1, outputDb = 0, order = 0;
    double sOn = 1, sFloor = 1, sLength = 8, sStuck = 0, sEnv = 0, sDisorder = 0.2, sRate = 0, sAbsence = 0, sRepeat = 0.2,
           sFreeze = 0, sDream = 1, sSync = 0, sLean = 0, sSmoo = 0.2, sMonitor = 0.5, sSustain = 0;
    double hOn = 0, hRaw = 0, hMemory = 0.5, hFreeze = 0, hSize = 300, hRate = 0, hLo = 0, hHi = 0, hTemp = 0.2, hWidth = 1, hWet = 0.5, hDry = 1;
    double xOn = 0, xMode = 0, xVerge = -30, xBoost = 0, xSmooth = 20, xRetain = 0, xDriveA = 0.2, xCutA = 0, xVolA = 0.7, xSide = 0,
           xDrift = 0.2, xMelt = 0.3, xDriveB = 0.2, xCutB = 0, xVolB = 0.5;

    bool set (const std::string& id, double v)
    {
        static const std::map<std::string, double Params::*> m {
            {"inputDb",&Params::inputDb},{"mix",&Params::mix},{"outputDb",&Params::outputDb},{"order",&Params::order},
            {"sOn",&Params::sOn},{"sFloor",&Params::sFloor},{"sLength",&Params::sLength},{"sStuck",&Params::sStuck},{"sEnv",&Params::sEnv},
            {"sDisorder",&Params::sDisorder},{"sRate",&Params::sRate},{"sAbsence",&Params::sAbsence},{"sRepeat",&Params::sRepeat},
            {"sFreeze",&Params::sFreeze},{"sDream",&Params::sDream},{"sSync",&Params::sSync},{"sLean",&Params::sLean},{"sSmoo",&Params::sSmoo},
            {"sMonitor",&Params::sMonitor},{"sSustain",&Params::sSustain},
            {"hOn",&Params::hOn},{"hRaw",&Params::hRaw},{"hMemory",&Params::hMemory},{"hFreeze",&Params::hFreeze},{"hSize",&Params::hSize},
            {"hRate",&Params::hRate},{"hLo",&Params::hLo},{"hHi",&Params::hHi},{"hTemp",&Params::hTemp},{"hWidth",&Params::hWidth},
            {"hWet",&Params::hWet},{"hDry",&Params::hDry},
            {"xOn",&Params::xOn},{"xMode",&Params::xMode},{"xVerge",&Params::xVerge},{"xBoost",&Params::xBoost},{"xSmooth",&Params::xSmooth},
            {"xRetain",&Params::xRetain},{"xDriveA",&Params::xDriveA},{"xCutA",&Params::xCutA},{"xVolA",&Params::xVolA},{"xSide",&Params::xSide},
            {"xDrift",&Params::xDrift},{"xMelt",&Params::xMelt},{"xDriveB",&Params::xDriveB},{"xCutB",&Params::xCutB},{"xVolB",&Params::xVolB} };
        auto it = m.find (id);
        if (it == m.end() || ! std::isfinite (v)) return false;
        this->*(it->second) = v; return true;
    }
};

//============================================================================== SHRED
static constexpr int kHist = 32;
struct ShredP
{
    int length = 8; double floorN = 0, maxN = 0, stuck = 0; bool envOn = false; double disorder = 0, rate = 1, absence = 0, repeat = 0;
    bool freeze = false, sustain = false; double forgetN = 0, envDecay = 0, dream = 1; bool sync = false; double lean = 0, smooK = 1, srcLvl = 1, prcLvl = 1;
};

struct ShredChannel
{
    std::vector<float> buf; Rng rng; uint32_t seed;
    double hs[kHist] {}, hl[kHist] {};
    int64_t w = 0, colStart = 0; double prev = 0; int ws = 0, hHead = 0, hCount = 0; double env = 0; int64_t quiet = 0;
    bool playing = false, hasCur = false, newCol = false; double cs = 0, cl = 0, pos = 0, g = 1, dream = 1, gate = 1;

    explicit ShredChannel (uint32_t s) : buf ((size_t) kBufN, 0.0f), rng (s), seed (s) { reset(); }
    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f); w = 0; prev = 0; quiet = 0; colStart = 0; ws = 0; hHead = 0; hCount = 0; env = 0;
        playing = false; cs = 0; cl = 0; pos = 0; g = 1; hasCur = false; dream = 1; gate = 1; newCol = false;
    }
    void push (double start, double len) { hs[hHead] = start; hl[hHead] = len; hHead = (hHead + 1) % kHist; if (hCount < kHist) hCount++; }
    bool valid (int k) const { return (double) w - hs[k] < kBufN - 4; }
    void record (double x, const ShredP& P)
    {
        buf[(size_t) (w & kBufMask)] = (float) x;
        const double ax = std::abs (x); env = ax > env ? ax : env * P.envDecay;
        const bool silent = env < 0.001;
        quiet = silent ? quiet + 1 : 0;
        if (! P.sustain && ! P.freeze && (double) quiet == P.forgetN) { hCount = 0; hasCur = false; }
        const bool hold = P.sustain && silent;
        if (hold) { colStart = w + 1; ws = 0; }
        else
        {
            const bool cross = prev < 0 && x >= 0;
            const int64_t len = w - colStart;
            if (cross) ws++;
            if ((cross && ws >= P.length && (double) len >= P.floorN) || (double) len >= P.maxN)
            {
                if (len > 0)
                    if (rng.next() >= P.stuck) push ((double) colStart, (double) len);
                colStart = w; ws = 0;
            }
        }
        prev = x; w++;
    }
    bool pick (const ShredP& P)
    {
        bool same = false;
        if (hasCur)
        {
            if (P.freeze) same = true;
            else if (rng.next() < P.repeat) same = true;
        }
        if (! same)
        {
            if (hCount == 0) { hasCur = false; return false; }
            int k = (hHead - 1 + kHist) % kHist;
            if (rng.next() < P.disorder)
            {
                const int j = (int) std::floor (rng.next() * hCount);
                const int kk = (hHead - 1 - j + 2 * kHist) % kHist;
                if (valid (kk)) k = kk;
            }
            cs = hs[k]; cl = hl[k]; hasCur = true;
        }
        const double a = P.absence;
        if (a < 0) g = 1 + a * rng.next();
        else if (a > 0) g = rng.next() < a ? 0 : 1;
        else g = 1;
        pos = 0; playing = true; newCol = true;
        return true;
    }
    double play (const ShredP& P)
    {
        if (! playing && ! pick (P)) return 0;
        if (pos >= cl && ! pick (P)) return 0;
        const double p = pos; const double fi = std::floor (p); const double fr = p - fi;
        const int64_t i = (int64_t) cs + (int64_t) fi;
        const double a = buf[(size_t) (i & kBufMask)], b = buf[(size_t) ((i + 1) & kBufMask)];
        double y = (a + (b - a) * fr) * g;
        if (P.envOn) { const double s = std::sin (kPi * p / cl); y *= s * s; }
        pos += P.rate;
        if (pos >= cl) playing = false;
        return y;
    }
};

struct Shred
{
    double sr = 44100; ShredChannel c0 { 0x1234567u }, c1 { 0x89ABCDEu }; Rng drng { 0x5EED5u }; ShredP P;
    void reset() { c0.reset(); c1.reset(); drng = Rng (0x5EED5u); }
    void set (const Params& p)
    {
        P.length = (int) jsRound (p.sLength); P.floorN = jsRound (kFloorMs[(int) clampd (jsRound (p.sFloor), 0, 3)] * 0.001 * sr);
        P.maxN = jsRound (sr * 0.75); P.stuck = p.sStuck; P.envOn = p.sEnv >= 0.5; P.disorder = p.sDisorder;
        P.rate = std::pow (2.0, p.sRate / 12.0); P.absence = p.sAbsence; P.repeat = p.sRepeat; P.freeze = p.sFreeze >= 0.5;
        P.sustain = p.sSustain >= 0.5; P.forgetN = jsRound (0.2 * sr); P.envDecay = std::exp (-1.0 / (0.05 * sr));
        P.dream = p.sDream; P.sync = p.sSync >= 0.5; P.lean = p.sLean;
        P.smooK = p.sSmoo <= 0 ? 1.0 : 1.0 - std::exp (-1.0 / (p.sSmoo * 0.06 * sr));
        P.srcLvl = std::min (1.0, 2 * (1 - p.sMonitor)); P.prcLvl = std::min (1.0, 2 * p.sMonitor);
    }
    void run (float* L, float* R, int n)
    {
        const double pL = P.dream * std::min (1.0, 1 - P.lean), pR = P.dream * std::min (1.0, 1 + P.lean);
        for (int i = 0; i < n; ++i)
        {
            const double xl = L[i], xr = R[i];
            c0.record (xl, P); c1.record (xr, P);
            c0.newCol = false; c1.newCol = false;
            const double yl = c0.play (P), yr = c1.play (P);
            if (c0.newCol) c0.dream = drng.next() < pL ? 1 : 0;
            if (P.sync) { if (c0.newCol) c1.dream = c0.dream; }
            else if (c1.newCol) c1.dream = drng.next() < pR ? 1 : 0;
            c0.gate += (c0.dream - c0.gate) * P.smooK; c1.gate += (c1.dream - c1.gate) * P.smooK;
            L[i] = (float) (xl * P.srcLvl * (1 - c0.gate) + yl * P.prcLvl * c0.gate);
            R[i] = (float) (xr * P.srcLvl * (1 - c1.gate) + yr * P.prcLvl * c1.gate);
        }
    }
};

//============================================================================== HAZE
static constexpr int kGrains = 4;
struct HazeP { bool raw = false, freeze = false; double mem = 0.5; int S = 1000; double gl = 500, rate = 1, temp = 0, kf1 = 0, kf2 = 0, ks = 0, kn = 0, width = 1, wet = 0.5, dry = 1; };

struct HazeChannel
{
    std::vector<float> buf; Rng rng; uint32_t seed; Biquad lo, hi;
    int wp = 0; double ef = 0, es = 0, nz = 0; double gs[kGrains] {}, gp[kGrains] {}; bool started = false;
    explicit HazeChannel (uint32_t s) : buf ((size_t) kBufN, 0.0f), rng (s), seed (s) { reset(); }
    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f); wp = 0; ef = es = nz = 0;
        for (int v = 0; v < kGrains; ++v) { gs[v] = 0; gp[v] = 0; }
        lo.clear(); hi.clear(); started = false;
    }
    double run (double x, const HazeP& P)
    {
        if (! P.raw)
        {
            const double a = std::abs (x);
            ef += (a - ef) * (a > ef ? P.kf1 : P.kf2);
            es += (a - es) * P.ks;
            x *= ef > 1e-9 ? std::min (1.0, es / ef) : 1.0;
        }
        const int S = P.S;
        if (wp >= S) wp = 0;
        if (! P.freeze) buf[(size_t) wp] = (float) (buf[(size_t) wp] * P.mem + x * (1 - P.mem));
        wp++;
        const double gl = P.gl;
        if (! started) { for (int v = 0; v < kGrains; ++v) { gs[v] = rng.next() * S; gp[v] = gl * v / kGrains; } started = true; }
        double y = 0;
        for (int v = 0; v < kGrains; ++v)
        {
            if (gp[v] >= gl) { gp[v] -= gl; gs[v] = rng.next() * S; }
            const double ph = gp[v];
            double r = gs[v] + ph * P.rate; r -= std::floor (r / S) * S;
            const int i = (int) std::floor (r); const double fr = r - i; const int i2 = i + 1 >= S ? 0 : i + 1;
            const double s = buf[(size_t) i] + ((double) buf[(size_t) i2] - (double) buf[(size_t) i]) * fr;
            const double ww = std::sin (kPi * ph / gl);
            y += s * ww * ww;
            gp[v] = ph + 1;
        }
        y *= 0.5;
        if (P.temp > 0)
        {
            const double d = 1 + 6 * P.temp;
            y = std::tanh (y * d) / (1 + 1.5 * P.temp);
            if (P.temp > 0.5)
            {
                nz += ((rng.next() * 2 - 1) - nz) * P.kn;
                const double k = (P.temp - 0.5) * 1.8;
                y *= 1 - k * (0.5 + 0.5 * clampd (nz * 4, -1, 1));
            }
        }
        return hi.run (lo.run (y));
    }
};

struct Haze
{
    double sr = 44100; HazeChannel c0 { 0x2468ACEu }, c1 { 0x13579BDu }; HazeP P; double eqLo = 1e9, eqHi = 1e9;
    void reset() { c0.reset(); c1.reset(); }
    void set (const Params& p)
    {
        P.raw = p.hRaw >= 0.5; P.mem = p.hFreeze >= 0.5 ? 1 : p.hMemory; P.freeze = p.hFreeze >= 0.5;
        P.S = (int) clampd (jsRound (p.hSize * 0.001 * sr), 64, kBufN - 1);
        P.gl = clampd (jsRound (P.S * 0.5), jsRound (0.01 * sr), jsRound (0.25 * sr));
        P.rate = std::pow (2.0, p.hRate / 12.0); P.temp = p.hTemp;
        P.kf1 = 1 - std::exp (-1.0 / (0.0005 * sr)); P.kf2 = 1 - std::exp (-1.0 / (0.03 * sr)); P.ks = 1 - std::exp (-1.0 / (0.025 * sr));
        P.kn = 1 - std::exp (-kTau * 40.0 / sr);
        P.width = p.hWidth; P.wet = p.hWet; P.dry = p.hDry;
        if (p.hLo != eqLo || p.hHi != eqHi)
        {
            eqLo = p.hLo; eqHi = p.hHi;
            for (auto* c : { &c0, &c1 }) { c->lo.shelf (sr, 250, p.hLo, false); c->hi.shelf (sr, 3500, p.hHi, true); }
        }
    }
    void run (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            double l = c0.run (L[i], P), r = c1.run (R[i], P);
            const double m = 0.5 * (l + r), s = 0.5 * (l - r) * P.width;
            l = m + s; r = m - s;
            L[i] = (float) (L[i] * P.dry + l * P.wet); R[i] = (float) (R[i] * P.dry + r * P.wet);
        }
    }
};

//============================================================================== SPLIT
struct SplitP
{
    int mode = 0; double verge = -30, boost = 1, pkRel = 0, ks = 0, retain = 0; int actN = 0; double burstP = 0, burstDecay = 0;
    double driveA = 0, cutA = 0, volA = 0, side = 0, drift = 0, melt = 0, driveB = 0, cutB = 0, volB = 0; int lfoN = 0; double lfoK = 0, d0 = 0, d1 = 0;
};

struct SplitChannel
{
    Rng rng; double sr; Svf lpg, cutA, cutB;
    std::vector<float> dl; int apN[4] {}; std::vector<float> ap[4]; int apI[4] {};
    double pk = 0, es = 0, g = 0, burst = 0; int act = 0; int64_t dw = 0; double lfo = 0, lfoT = 0; int lfoC = 0;

    SplitChannel (uint32_t seed, double sampleRate) : rng (seed), sr (sampleRate), dl (4096, 0.0f)
    {
        static constexpr double ms[4] = { 3.2, 2.4, 8.6, 6.3 };
        for (int k = 0; k < 4; ++k) { apN[k] = (int) jsRound (ms[k] * 0.001 * sr); ap[k].assign ((size_t) apN[k], 0.0f); }
        reset();
    }
    void reset()
    {
        pk = es = g = burst = 0; act = 0; lpg.clear(); cutA.clear(); cutB.clear();
        std::fill (dl.begin(), dl.end(), 0.0f); dw = 0; lfo = lfoT = 0; lfoC = 0;
        for (int k = 0; k < 4; ++k) { std::fill (ap[k].begin(), ap[k].end(), 0.0f); apI[k] = 0; }
    }
    void detect (double x, const SplitP& P)
    {
        const double a = std::abs (x) * P.boost;
        pk = a > pk ? a : pk * P.pkRel;
        es += (pk - es) * P.ks;
        const double db = 20 * std::log10 (es + 1e-12);
        const double gg = clampd ((db - P.verge) / 24, 0, 1);
        if (gg > 0.05) act = P.actN;
        else if (act > 0) act--;
        if (P.retain > 0 && act > 0 && rng.next() < P.burstP) burst = std::max (burst, rng.next() * P.retain);
        burst *= P.burstDecay;
        g = gg > burst ? gg : burst;
    }
    double above (double x, double gv, const SplitP& P, bool upd)
    {
        if (upd) lpg.set (sr, 60 * std::pow (2.0, gv * 8.3), 0.8);
        double y = lpg.run (x) * std::min (1.0, gv * 4);
        if (P.driveA > 0) { const double k = 1 + 15 * P.driveA; y = std::tanh (y * k) / (1 + 2 * P.driveA); }
        if (P.cutA > 0) { cutA.run (y); y = cutA.hp; }
        return y * P.volA;
    }
    double below (double x, double gv, const SplitP& P)
    {
        double y = x * (1 - std::min (1.0, gv * (1 + 15 * P.side)));
        dl[(size_t) (dw & 4095)] = (float) y;
        if (P.drift > 0)
        {
            if (--lfoC <= 0) { lfoC = P.lfoN; lfoT = rng.next(); }
            lfo += (lfoT - lfo) * P.lfoK;
            const double d = P.drift * (P.d0 + P.d1 * lfo);
            const double r = (double) dw - d; const double fi = std::floor (r); const double fr = r - fi;
            const int64_t i = (int64_t) fi;
            const double a = dl[(size_t) (i & 4095)], b = dl[(size_t) ((i + 1) & 4095)];
            y = a + (b - a) * fr;
        }
        dw++;
        if (P.melt > 0)
        {
            double z = y;
            for (int k = 0; k < 4; ++k)
            {
                auto& b = ap[k]; const int j = apI[k]; const double v = b[(size_t) j];
                const double u = z + 0.7 * v; b[(size_t) j] = (float) u; z = v - 0.7 * u;
                apI[k] = j + 1 >= apN[k] ? 0 : j + 1;
            }
            y = y * (1 - P.melt) + z * P.melt;
        }
        if (P.driveB > 0) { const double k = 1 + 15 * P.driveB; y = std::tanh (y * k) / (1 + 2 * P.driveB); }
        if (P.cutB > 0) y = cutB.run (y);
        return y * P.volB;
    }
};

struct Split
{
    double sr; SplitChannel c0, c1; SplitP P; uint32_t cnt = 0;
    explicit Split (double s) : sr (s), c0 (0x3C6EF37u, s), c1 (0x0BB67AEu, s) {}
    void reset() { c0.reset(); c1.reset(); cnt = 0; }
    void set (const Params& p)
    {
        P.mode = (int) jsRound (p.xMode); P.verge = p.xVerge; P.boost = dbToGain (p.xBoost);
        P.pkRel = std::exp (-1.0 / (0.05 * sr)); P.ks = 1 - std::exp (-1.0 / (p.xSmooth * 0.001 * sr));
        P.retain = p.xRetain; P.actN = (int) jsRound (sr * 1.0); P.burstP = 20.0 / sr * (0.3 + p.xRetain);
        P.burstDecay = std::exp (-1.0 / (0.06 * sr));
        P.driveA = p.xDriveA; P.cutA = p.xCutA; P.volA = p.xVolA; P.side = p.xSide;
        P.drift = p.xDrift; P.melt = p.xMelt; P.driveB = p.xDriveB; P.cutB = p.xCutB; P.volB = p.xVolB;
        P.lfoN = (int) jsRound (0.08 * sr); P.lfoK = 1 - std::exp (-1.0 / (0.04 * sr)); P.d0 = 0.003 * sr; P.d1 = 0.010 * sr;
        const double fa = 20 * std::pow (150.0, p.xCutA), fb = 20000 * std::pow (0.015, p.xCutB);
        for (auto* c : { &c0, &c1 }) { c->cutA.set (sr, fa, 0.707); c->cutB.set (sr, fb, 0.707); }
    }
    void run (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const double xl = L[i], xr = R[i];
            c0.detect (xl, P); c1.detect (xr, P);
            double gl = c0.g, gr = c1.g;
            if (P.mode == 1) std::swap (gl, gr);
            else if (P.mode == 2) gl = gr = 0.5 * (gl + gr);
            const bool upd = (cnt++ & 31u) == 0;
            L[i] = (float) (c0.above (xl, gl, P, upd) + c0.below (xl, gl, P));
            R[i] = (float) (c1.above (xr, gr, P, upd) + c1.below (xr, gr, P));
        }
    }
};

//============================================================================== cadena
struct Engine
{
    double sr; Shred shred; Haze haze; Split split; Params p; bool dirty = true;
    double gIn = 1, gOut = 1, gMix = 1, kSm; std::vector<float> dL, dR; bool first = true;

    explicit Engine (double sampleRate, int maxBlock = 4096) : sr (sampleRate), split (sampleRate)
    {
        shred.sr = sr; haze.sr = sr; kSm = 1 - std::exp (-1.0 / (0.02 * sr));
        dL.assign ((size_t) maxBlock, 0.0f); dR.assign ((size_t) maxBlock, 0.0f);
    }
    void setParams (const Params& np) { p = np; dirty = true; }
    void reset() { shred.reset(); haze.reset(); split.reset(); first = true; }
    // procesa en sitio; n <= maxBlock (si no, por trozos)
    void process (float* L, float* R, int n)
    {
        const int cap = (int) dL.size();
        for (int o = 0; o < n; o += cap) block (L + o, R + o, std::min (cap, n - o));
    }
    void leds (double out[4]) const
    {
        out[0] = p.sOn >= 0.5 ? shred.c0.gate : 0; out[1] = p.sOn >= 0.5 ? shred.c1.gate : 0;
        out[2] = p.xOn >= 0.5 ? split.c0.g : 0; out[3] = p.xOn >= 0.5 ? split.c1.g : 0;
    }
private:
    void block (float* L, float* R, int n)
    {
        if (dirty) { shred.set (p); haze.set (p); split.set (p); dirty = false; }
        const double tIn = dbToGain (p.inputDb), tOut = dbToGain (p.outputDb), tMix = p.mix, k = kSm;
        if (first) { gIn = tIn; gOut = tOut; gMix = tMix; first = false; }
        double gi = gIn;
        for (int i = 0; i < n; ++i) { gi += (tIn - gi) * k; L[i] = (float) (L[i] * gi); R[i] = (float) (R[i] * gi); dL[(size_t) i] = L[i]; dR[(size_t) i] = R[i]; }
        gIn = gi;
        const int oi = (int) clampd (jsRound (p.order), 0, 5);
        for (int j = 0; j < 3; ++j)
        {
            const int m = kOrders[oi][j];
            if (m == 0 && p.sOn >= 0.5) shred.run (L, R, n);
            else if (m == 1 && p.hOn >= 0.5) haze.run (L, R, n);
            else if (m == 2 && p.xOn >= 0.5) split.run (L, R, n);
        }
        double gm = gMix, go = gOut;
        for (int i = 0; i < n; ++i)
        {
            gm += (tMix - gm) * k; go += (tOut - go) * k;
            double l = (dL[(size_t) i] * (1 - gm) + L[i] * gm) * go, r = (dR[(size_t) i] * (1 - gm) + R[i] * gm) * go;
            if (! (l == l) || ! (r == r)) { l = 0; r = 0; }
            L[i] = (float) l; R[i] = (float) r;
        }
        gMix = gm; gOut = go;
    }
};
} // namespace glitch
