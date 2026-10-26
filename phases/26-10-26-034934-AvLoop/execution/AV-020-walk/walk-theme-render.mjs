// AV-020's render walk (D-018): the colour theme in the export's own frames, rendered by avloop-cli frame from projects
// of the counter sample (six colour bars over a black strip). Replace yellow → red: the yellow bar red, the red and
// green bars (60° away) byte for byte as without; Colourise red: the blue bar red; Duotone: the black strip the dark
// colour; an AV-019 file without the theme's keys renders as none; the GPU and the CPU draw the same picture.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-theme-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const W = 640;

function project(name, effects) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 2,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 },
    loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
    effects,
  }));
  return file;
}

function frame(file, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', '0', '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  return r.status === 0 ? fs.readFileSync(raw) : null;
}
const px = (buf, [x, y]) => [...buf.subarray((y * W + x) * 4, (y * W + x) * 4 + 4)];
const same = (a, b) => a.every((v, i) => v === b[i]);
const maxDiff = (a, b) => a.reduce((m, v, i) => Math.max(m, Math.abs(v - b[i])), 0);
const RED = [40, 250];
const GREEN = [160, 250];
const YELLOW = [266, 250];
const BLUE = [373, 250];
const STRIP = [300, 345]; // the black strip under the bars

const none = frame(project('none', { exposure: 0 }));
w.check(none !== null, 'an AV-019 file without the theme\'s keys renders', none ? `${none.length} bytes` : 'no frame');
w.check(px(none, YELLOW)[0] > 230 && px(none, YELLOW)[1] > 230 && px(none, YELLOW)[2] < 30, 'the yellow bar as it is',
  px(none, YELLOW).join(','));

const replaced = frame(project('replace', { replace: 100, replaceFrom: '#ffff00', replaceTo: '#ff0000' }));
const y = px(replaced, YELLOW);
w.check(y[0] > 230 && y[1] < 30 && y[2] < 30, 'Replace yellow → red: the yellow bar red', y.join(','));
w.check(same(px(replaced, RED), px(none, RED)) && same(px(replaced, GREEN), px(none, GREEN)),
  'the red and green bars (60° away) untouched', `${px(replaced, RED).join(',')} | ${px(replaced, GREEN).join(',')}`);

const red = px(frame(project('colourise', { colourise: 100, colouriseColour: '#e02020' })), BLUE);
w.check(red[0] > red[1] + 10 && Math.abs(red[1] - red[2]) <= 2, 'Colourise red: the blue bar red', red.join(','));

const strip = px(frame(project('duotone', { duotone: 100, duotoneDark: '#000040', duotoneLight: '#ffff80' })), STRIP);
w.check(strip[0] <= 3 && strip[1] <= 3 && Math.abs(strip[2] - 64) <= 3, 'Duotone: black becomes #000040', strip.join(','));

const theme = { replace: 100, replaceFrom: '#ffff00', replaceTo: '#ff0080', split: 70, colourise: 30, duotone: 20 };
const gpu = frame(project('theme', theme));
const cpu = frame(project('theme', theme), true);
w.check(gpu && cpu && maxDiff(gpu, cpu) <= 4, 'a whole theme: the GPU and the CPU draw the same',
  gpu && cpu ? `max diff ${maxDiff(gpu, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'replace.avloop.rgba.png'), path.join(HERE, 'render-replaced.png'));
fs.copyFileSync(path.join(TMP, 'theme.avloop.rgba.png'), path.join(HERE, 'render-theme.png'));

fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
