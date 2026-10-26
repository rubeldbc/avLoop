// AV-019's render walk (D-017): the clip effects in the export's own frames, rendered by avloop-cli frame — the
// FrameRenderer the export and the Preview use — from projects of the counter sample (six colour bars). A pixel of the
// red bar: as it is without effects (a version-1 file), cyan when inverted, grey of red's luma at Saturation 0, half
// see-through at Opacity 50 %; Off with values set draws the frame byte for byte as none; Blur mixes the green bar into
// the red one's edge; the GPU and the CPU draw the same picture (max 4/255).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-effects-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const W = 640;

function project(name, effects) {
  const file = path.join(TMP, `${name}.avloop`);
  const json = {
    format: 'avloop', version: effects ? 2 : 1,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 },
    loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  };
  if (effects) json.effects = effects;
  fs.writeFileSync(file, JSON.stringify(json));
  return file;
}

// Output frame 0's premultiplied RGBA bytes.
function frame(file, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', '0', '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  return r.status === 0 ? fs.readFileSync(raw) : null;
}
const px = (buf, x, y) => [...buf.subarray((y * W + x) * 4, (y * W + x) * 4 + 4)];
const near = (a, b, tol) => a.every((v, i) => Math.abs(v - b[i]) <= tol);
const maxDiff = (a, b) => a.reduce((m, v, i) => Math.max(m, Math.abs(v - b[i])), 0);
const RED = [40, 250]; // the red bar, under the moving stripe and away from the counter

const none = frame(project('none', null));
w.check(none && none.length === W * 360 * 4, 'a version-1 project renders as before', none ? `${none.length} bytes` : 'no frame');
const red = px(none, ...RED);
w.check(red[0] > 230 && red[1] < 25 && red[2] < 25 && red[3] === 255, 'the red bar without effects', red.join(','));

const inverted = frame(project('invert', { invert: true }));
const cyan = px(inverted, ...RED);
w.check(near(cyan, [255 - red[0], 255 - red[1], 255 - red[2], 255], 1), 'Invert: red turns cyan', cyan.join(','));

const greyed = px(frame(project('grey', { saturation: 0 })), ...RED);
w.check(Math.abs(greyed[0] - greyed[1]) <= 1 && Math.abs(greyed[1] - greyed[2]) <= 1 && Math.abs(greyed[0] - 54) <= 8,
  'Saturation 0: the grey of red\'s luma (≈ 54)', greyed.join(','));

const half = px(frame(project('half', { opacity: 50 })), ...RED);
w.check(Math.abs(half[3] - 128) <= 1 && Math.abs(half[0] - red[0] / 2) <= 2, 'Opacity 50 %: alpha and colour halved',
  half.join(','));

const off = frame(project('off', { on: false, invert: true, exposure: 2, blur: 20 }));
w.check(off && Buffer.compare(off, none) === 0, 'Off: byte for byte the frame without effects',
  off ? `max diff ${maxDiff(off, none)}` : 'no frame');

// The red | green edge (x ≈ 107): blurred, green reaches into the red bar.
const blurred = frame(project('blur', { blur: 20 }));
const edge = px(blurred, 100, 250);
w.check(edge[1] > 30 && px(none, 100, 250)[1] < 25, 'Blur 20 px: green spread into the red bar\'s edge',
  `${px(none, 100, 250).join(',')} → ${edge.join(',')}`);

const look = { exposure: 0.6, contrast: 20, temperature: 30, hue: 25, vibrance: 40, vignette: 50, sharpen: 30, blur: 2 };
const gpu = frame(project('look', look));
const cpu = frame(project('look', look), true);
w.check(gpu && cpu && maxDiff(gpu, cpu) <= 4, 'a whole look: the GPU and the CPU draw the same',
  gpu && cpu ? `max diff ${maxDiff(gpu, cpu)}` : 'no frame');
w.check(maxDiff(gpu, none) > 40, 'the look changed the picture', `max diff from none ${maxDiff(gpu, none)}`);
fs.copyFileSync(path.join(TMP, 'look.avloop.rgba.png'), path.join(HERE, 'render-look.png'));
fs.copyFileSync(path.join(TMP, 'invert.avloop.rgba.png'), path.join(HERE, 'render-invert.png'));

fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
