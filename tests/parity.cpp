// Paridad JS ↔ C++: lee entrada (float32 estéreo intercalado) y parámetros "id valor" por línea, escribe la salida.
// g++ -O2 -std=c++17 tests/parity.cpp -o /tmp/gh_parity && node tests/parity.js /tmp/gh_parity
#include "../vst/Source/dsp/GlitchDSP.h"
#include <cstdio>
#include <fstream>
#include <sstream>
int main (int argc, char** argv)
{
    if (argc < 6) { std::fprintf (stderr, "uso: parity sr in.f32 params.txt out.f32 bloque\n"); return 1; }
    const double sr = std::atof (argv[1]); const int B = std::atoi (argv[5]);
    std::ifstream fi (argv[2], std::ios::binary); std::vector<float> in ((std::istreambuf_iterator<char> (fi)), {});
    std::vector<float> raw; { std::ifstream f (argv[2], std::ios::binary); f.seekg (0, std::ios::end); size_t sz = (size_t) f.tellg(); f.seekg (0); raw.resize (sz / 4); f.read ((char*) raw.data(), (std::streamsize) sz); }
    const int n = (int) raw.size() / 2; std::vector<float> L ((size_t) n), R ((size_t) n);
    for (int i = 0; i < n; ++i) { L[(size_t) i] = raw[(size_t) (2 * i)]; R[(size_t) i] = raw[(size_t) (2 * i + 1)]; }
    glitch::Params p; std::ifstream fp (argv[3]); std::string id; double v;
    while (fp >> id >> v) if (! p.set (id, v)) { std::fprintf (stderr, "parámetro desconocido %s\n", id.c_str()); return 2; }
    glitch::Engine e (sr, 4096); e.setParams (p);
    for (int o = 0; o < n; o += B) { const int m = std::min (B, n - o); e.process (L.data() + o, R.data() + o, m); }
    std::ofstream fo (argv[4], std::ios::binary);
    for (int i = 0; i < n; ++i) { fo.write ((const char*) &L[(size_t) i], 4); fo.write ((const char*) &R[(size_t) i], 4); }
    return 0;
}
