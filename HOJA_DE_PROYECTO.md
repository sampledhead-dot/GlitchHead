# GlitchHead — Hoja de proyecto

Familia de apps de **Sampled Head**. **Versión actual:** web 1.0.0 · VST 1.0.0 (20-11-2026)
**Repositorio:** https://github.com/sampledhead-dot/GlitchHead · **Web:** https://sampledhead-dot.github.io/GlitchHead/

## Descripción
Tres efectos «salvajes» en cadena, inspirados en la funcionalidad de una serie de plugins comerciales de efectos
experimentales (manual de referencia del usuario, 20-11-2026), con nombre, diseño visual y código propios.
No se usan marcas, nombres de producto ni diseño del original.

| Motor | Qué hace |
|---|---|
| **SHRED** | Graba la entrada sin parar y la corta en cada cruce por cero ascendente (wavesets). Junta `Length` wavesets (y al menos `Floor` ms) en una **colección** y las reproduce con probabilidades: `Stuck` (no grabar colecciones nuevas), `Disorder` (saltar a una de las 32 anteriores), `Repeat`/`Freeze`, `Absence` (izq.: volumen aleatorio; der.: silencio aleatorio), `Rate` (semitonos), `Envelope` (ventana de Hann). **Dream**: en cada colección se sortea si suena el proceso o la fuente; `Smoo` (fundido), `Lean` (reparto L/R), `Sync` (mono), `Monitor` (balance). `Sustain`: sin entrada sigue sonando; sin él, tras 0,2 s de silencio se olvida la memoria. |
| **HAZE** | Memoria circular de `Size` (20 ms–2 s): se escribe encima mezclando lo nuevo con lo viejo (`Memory`; `Freeze` = no escribir). La leen 4 granos con ventana de Hann escalonados (longitud = Size/2, entre 10 y 250 ms) a `Rate`. `Raw` apagado suaviza los ataques de la entrada. `Temperature` (tanh y, en la mitad alta, AM con ruido filtrado a 40 Hz), estantes `Lo` 250 Hz / `Hi` 3,5 kHz ±12 dB, `Mid/Side`, `Wet`, `Dry`. |
| **SPLIT** | Puerta partida. Detector de pico (caída 50 ms) suavizado por `Smooth`; la envolvente es lo que pasa de `Verge` (en dB, 0..24 dB → 0..1), con `Boost` al detector y `Retain` (ráfagas aleatorias en la caída). `Mode`: Direct / Crossed (L↔R) / Summed. **ARRIBA**: low-pass gate (SVF 60 Hz → 19 kHz con la envolvente) + `Drive` + `Cut` (paso alto 20 Hz→3 kHz) + `Vol`. **ABAJO**: amplitud inversa (con `Sidechain` agacha más) + `Drift` (retardo aleatorio 0–13 ms) + `Melt` (4 pasatodos) + `Drive` + `Cut` (paso bajo 20 kHz→300 Hz) + `Vol`. Sin mezcla: es un coloreador. |

Globales: `Input`, `Mix`, `Output` y **orden** de los tres (6 órdenes). 20 presets de fábrica (Shred, Haze, Split y
combinaciones) + presets de usuario. **Candado**: al cambiar de preset se conservan Input, Mix y Output.

## Dónde está
1. **Plugin** (`vst/`): VST3 (Win/Mac), AU (Mac) y Standalone, JUCE 9.0.5, tiempo real. Releases `vst-vX.Y.Z`.
2. **App web** (`index.html`): subir audio/vídeo, grabar, «En directo» (micro/interfaz en tiempo real con AudioWorklet),
   loop de prueba, reproducir con A/B, exportar WAV 24 bits (procesado entero, con cola opcional de 2 s), grabar la salida,
   «⇄ Apps» (recibe loops, stems, pads y frases; envía el audio procesado).
3. **CrazyHead** (pestaña GlitchHead, 1.21) y **SampledHead App** (pestaña GlitchHead): mismo motor y mismo panel.
4. **Plataforma SampledHead** (Windows/Mac): app + plugin en la lista.

## Arquitectura
```
Input ─┬──────────────────────────────── seco ─────────────────────────────┐
       └─ [SHRED] → [HAZE] → [SPLIT]   (orden según «Orden», cada uno On/Off) ┴→ Mix → Output
```
- Todo el azar sale de `Rng` (xorshift32) con semillas fijas por canal y motor: **el JS y el C++ dan exactamente la misma
  salida** (diferencia 0 en `tests/parity.js`) y el resultado no depende del tamaño de bloque.
- Búferes de 2^18 muestras por canal y motor (~5,4 s a 48 kHz); el plugin los reserva en `prepareToPlay`.
- CPU medida: ~1,5 % de un núcleo con los tres motores en estéreo a 48 kHz (Linux, plugin).

### Archivos
| Ruta | Contenido |
|---|---|
| `dsp/glitchhead.js` | Motor de referencia (JS): `PARAMS` (id, nombre, rango, defecto, tipo, descripción), `PRESETS`, `Engine`, `processBuffers`, `workletSource()`/`engineSource()` |
| `web/gh-ui.js` | Panel de mandos web común (`GlitchHeadUI.mount`), nodo en tiempo real (`createNode`), procesado en Worker (`renderOffline`) |
| `web/index.template.html` | App web (fuente) |
| `build_web.js` | Une plantilla + motor + panel → `index.html` y `dist/glitchhead-vX.Y.Z.html` (standalone) |
| `vst/Source/dsp/GlitchDSP.h` | Motor en C++ (mismo diseño) |
| `vst/Source/ParamTable.h`, `Presets.h` | **Generados** por `node tests/gen_cpp.js` desde `PARAMS`/`PRESETS` |
| `vst/Source/PluginProcessor.*`, `PluginEditor.*`, `UpdateBanner.h` | Plugin |
| `tests/test_dsp.js` | Pruebas del motor (presets, bloques, Sustain, Freeze, Split) |
| `tests/parity.cpp/.js` | Paridad JS ↔ C++ |
| `tests/test_web.py` | Prueba de la web en Chrome headless (Playwright) |
| `vst/tests/TestMain.cpp` | Pruebas del plugin (paridad, mono, estabilidad, CPU, estado, candado, captura del editor) |
| `.github/workflows/build-vst.yml` | Pruebas + compilación Windows y Mac universal; release `vst-vX.Y.Z` |

## Reglas permanentes
- Versionado semántico. Web: `APP_VERSION` en `web/index.template.html` y `C` en `sw.js`. Plugin: `project(... VERSION)` en
  `vst/CMakeLists.txt` **y** `kVersion` en `PluginProcessor.h`. Motor: `VERSION` en `dsp/glitchhead.js`.
- **Paridad**: cualquier cambio de sonido en `dsp/glitchhead.js` **y** `GlitchDSP.h`; parámetros y presets solo en el JS y
  luego `node tests/gen_cpp.js`. El workflow falla si los generados no están al día.
- Al cambiar el motor o el panel, copiar `dsp/glitchhead.js` y `web/gh-ui.js` a CrazyHead y SampledHead App (`glitchhead/`).
- Entrega numerada `glitchhead-vX.Y.Z.zip` con carpeta `glitchhead-vX.Y.Z`, standalone e informe de error .txt.
- Probar antes de entregar: `node tests/test_dsp.js`, paridad, `tests/test_web.py`, `GlitchHeadTests`.
- Nombre **GlitchHead** y estética Sampled Head; no usar marcas, nombres ni diseño del producto de referencia.

## Historial
- **1.0.1** (24-11-2026): botón ☰ arriba a la derecha con el **menú común «☰ Menú»** de todas las apps (sh-core 1.5.0, `SH.ui.menu`): mismas secciones desplegables y en el mismo orden — Proyecto · Importar · Exportar · Otras apps · Copia de tus datos · Ajustes · Ayuda (guía e informe .txt) —; recuerda la sección abierta; los botones de siempre siguen haciendo lo mismo (el menú los pulsa); «⇄ Apps» va dentro (Otras apps, con el número de recibidos en el botón ☰). Presets en Proyecto.
- **1.0.0** (20-11-2026): primera versión. Motor de tres efectos, plugin VST3/AU/Standalone, app web, 20 presets,
  pestañas en CrazyHead y SampledHead App, conexión con las demás apps.

## Límites conocidos
- Diseño propio a partir de la descripción del manual: no es una réplica del sonido original.
- SHRED reproduce con ~1 colección de retraso (no se declara latencia: es parte del efecto).
- En la web «En directo» tiene la latencia del navegador (~20–60 ms); para tocar en directo, mejor el plugin o el Standalone.
- No probado: dentro de un DAW real, Windows/Mac reales, Safari/iOS, móviles reales; **los presets no se han escuchado con
  música real** (solo medidos con material sintético): ajustarlos al oído.
- La web usa `blob:` para el worklet y el Worker; desde `file://` en Chrome cae a `data:` y al hilo principal.
