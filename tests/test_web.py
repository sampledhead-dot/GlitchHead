# Prueba de extremo a extremo de la app web en Chrome headless (Playwright).
# python3 tests/test_web.py <index.html> <carpeta de salida>
import asyncio, sys, os, wave, struct
from playwright.async_api import async_playwright

HTML = os.path.abspath(sys.argv[1]); OUT = sys.argv[2]
os.makedirs(OUT, exist_ok=True)

async def main():
    fails = []
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path='/usr/bin/google-chrome', args=['--autoplay-policy=no-user-gesture-required', '--use-fake-ui-for-media-stream', '--use-fake-device-for-media-stream'])
        for vw, name in [(1280, 'escritorio'), (412, 'movil')]:
            ctx = await b.new_context(accept_downloads=True, viewport={'width': vw, 'height': 900}, device_scale_factor=1 if vw > 800 else 2)
            pg = await ctx.new_page()
            errs = []
            pg.on('pageerror', lambda e: errs.append(str(e)))
            pg.on('console', lambda m: errs.append(m.text) if m.type == 'error' else None)
            await pg.goto('file://' + HTML)
            await pg.wait_for_timeout(300)
            # loop de prueba
            await pg.click('#demoBtn')
            await pg.wait_for_function("document.getElementById('status').textContent.startsWith('Listo')", timeout=20000)
            # reproducir en tiempo real y ver que el worklet manda LED
            await pg.click('#playBtn')
            await pg.wait_for_timeout(1200)
            leds = await pg.evaluate("[...document.querySelectorAll('.gh-led')].map(l => +l.style.opacity || 0)")
            print(name, 'LED tras reproducir:', leds)
            if max(leds) <= 0.2: fails.append(name + ': los LED no se mueven (¿no llega el audio al worklet?)')
            # recorrer todos los presets de fábrica
            n = await pg.evaluate("document.querySelector('.gh-sel').options.length")
            for i in range(n):
                await pg.select_option('.gh-sel', index=i)
                await pg.wait_for_timeout(60)
            # pulsar todos los interruptores y pads dos veces
            for sel in ['.gh-sw', '.gh-pad']:
                cnt = await pg.locator(sel).count()
                for k in range(cnt):
                    await pg.locator(sel).nth(k).click()
                    await pg.locator(sel).nth(k).click()
            # mover un mando arrastrando
            box = await pg.locator('.gh-k svg').nth(4).bounding_box()
            await pg.mouse.move(box['x'] + box['width'] / 2, box['y'] + box['height'] / 2)
            await pg.mouse.down(); await pg.mouse.move(box['x'] + box['width'] / 2, box['y'] - 40, steps=5); await pg.mouse.up()
            info = await pg.text_content('.gh-info')
            print(name, 'info:', info[:70])
            # reordenar
            await pg.locator('.gh-mod').nth(0).locator('.gh-mv').nth(1).click()
            print(name, 'cadena:', await pg.text_content('.gh-flow'))
            # preset «Sombra granular» (los tres motores) y exportar WAV
            await pg.select_option('.gh-sel', label='Sombra granular')
            await pg.check('#tailChk')
            async with pg.expect_download(timeout=60000) as dl:
                await pg.click('#dlBtn')
            d = await dl.value
            path = os.path.join(OUT, name + '-' + d.suggested_filename); await d.save_as(path)
            with wave.open(path) as w:
                fr, ch, sw, sr = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
            print(name, 'WAV:', os.path.basename(path), fr, 'muestras', ch, 'canales', sw * 8, 'bits', sr, 'Hz')
            if ch != 2 or sw != 3 or fr < sr * 5.5: fails.append(name + ': WAV raro')
            # grabar la salida 1 s
            await pg.click('#outRecBtn'); await pg.wait_for_timeout(1000)
            async with pg.expect_download(timeout=20000) as dl2:
                await pg.click('#outRecBtn')
            d2 = await dl2.value
            p2 = os.path.join(OUT, name + '-salida.wav'); await d2.save_as(p2)
            with wave.open(p2) as w: print(name, 'salida grabada:', round(w.getnframes() / w.getframerate(), 2), 's')
            # en directo con micro falso
            await pg.click('#liveBtn'); await pg.wait_for_timeout(800)
            print(name, 'directo:', await pg.text_content('#status'))
            await pg.click('#liveBtn')
            # dado
            await pg.click('[data-a="rand"]')
            await pg.wait_for_timeout(300)
            await pg.screenshot(path=os.path.join(OUT, name + '.png'), full_page=True)
            # informe manual
            async with pg.expect_download(timeout=10000) as dl3:
                await pg.click('#reportBtn')
            r = await dl3.value; rp = os.path.join(OUT, name + '-informe.txt'); await r.save_as(rp)
            txt = open(rp, encoding='utf-8').read()
            if 'INFORME DE ERROR — GlitchHead' not in txt or 'Cronología' not in txt: fails.append(name + ': informe incompleto')
            errs = [e for e in errs if 'sw.js' not in e and 'favicon' not in e and 'manifest' not in e.lower() and 'sh-core' not in e and 'ERR_FILE_NOT_FOUND' not in e]
            if errs: fails.append(name + ': errores en la página: ' + ' | '.join(errs[:5]))
            await ctx.close()
        await b.close()
    print('FALLOS:\n' + '\n'.join(fails) if fails else 'TODO OK')
    sys.exit(1 if fails else 0)

asyncio.run(main())
