// Copia el motor y el panel de GlitchHead dentro de otra app de un solo archivo (CrazyHead, SampledHead App…).
// Sustituye lo que haya entre <!--GH:BEGIN--> y <!--GH:END--> (si no existe, lo inserta antes de </body>).
// node tools/embed.js ../CrazyHead/index.html ../sampledhead-app/index.html
const fs = require('fs'), path = require('path');
const R = f => fs.readFileSync(path.join(__dirname, '..', f), 'utf8');
const dsp = R('dsp/glitchhead.js'), ui = R('web/gh-ui.js');
const ver = (dsp.match(/VERSION: '([^']+)'/) || [])[1];
const block = `<!--GH:BEGIN motor y panel de GlitchHead ${ver} (copiados con GlitchHead/tools/embed.js: no editar aquí)-->\n<script>${dsp}</script>\n<script>${ui}</script>\n<!--GH:END-->`;
for (const f of process.argv.slice(2)) {
  let s = fs.readFileSync(f, 'utf8');
  const a = s.indexOf('<!--GH:BEGIN'), b = s.indexOf('<!--GH:END-->');
  if (a >= 0 && b > a) s = s.slice(0, a) + block + s.slice(b + '<!--GH:END-->'.length);
  else { const i = s.lastIndexOf('</body>'); if (i < 0) throw new Error(f + ': sin </body>'); s = s.slice(0, i) + block + '\n' + s.slice(i); }
  fs.writeFileSync(f, s); console.log('✔ ' + f + ' (motor ' + ver + ')');
}
