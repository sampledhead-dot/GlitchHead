// Une la plantilla + motor + panel → index.html (GitHub Pages) y dist/glitchhead-vX.Y.Z.html (standalone)
// node build_web.js
const fs = require('fs'), path = require('path');
const R = f => fs.readFileSync(path.join(__dirname, f), 'utf8');
const tpl = R('web/index.template.html'), dsp = R('dsp/glitchhead.js'), ui = R('web/gh-ui.js');
for (const [n, s] of [['dsp', dsp], ['ui', ui]]) if (/<\/script/i.test(s)) throw new Error(n + ' contiene </script');
const out = tpl.replace('/*__DSP__*/', () => dsp).replace('/*__UI__*/', () => ui);
const ver = (out.match(/APP_VERSION = '([^']+)'/) || [])[1];
fs.mkdirSync(path.join(__dirname, 'dist'), { recursive: true });
for (const f of fs.readdirSync(path.join(__dirname, 'dist'))) if (/^glitchhead-v.*\.html$/.test(f)) fs.unlinkSync(path.join(__dirname, 'dist', f));
fs.writeFileSync(path.join(__dirname, 'dist', 'glitchhead-v' + ver + '.html'), out);
fs.writeFileSync(path.join(__dirname, 'index.html'), out);
console.log('ok v' + ver);
