// AV-033's render walk (D-029): a layer's keyframes and effects in the export's own frames. The counter with itself as
// a half-size layer keyed from the top-left quarter (0 s) to the centre (2 s): frame 0 shows it at (100, 125) (its
// green over the main red), frame 30 (1 s) half way — the main video's red back at (100, 125) and the layer's red over
// the main green at (200, 150); opacity keyed 100 → 0: half at 1 s; Invert on the layer alone: its green made magenta,
// the main video untouched; Ease: at 0.5 s nearer the start than linear; a layer with neither: AV-030's bytes; GPU = CPU.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-layer-keys-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const layer = (extra) => ({ name: 'counter', path: sample, w: 640, h: 360, duration: 10, track: 2, start: 0, in: 0,
  out: 10, x: 160, y: 90, scale: 0.5, opacity: 100, visible: true, locked: false, ...extra });
const moving = [[0, 160, 90, 0.5, 100, 0], [2, 480, 270, 0.5, 100, 0]];
function project(name, layers) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 3, source: { path: sample, stream: 0 }, range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' }, layers,
  }));
  return file;
}
function frame(file, k, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}-${k}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', String(k), '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  const m = /(\d+)x(\d+)/.exec(r.out);
  return r.status === 0 ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : null;
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);
const red = (p) => p[0] > 200 && p[1] < 50 && p[2] < 50;
const green = (p) => p[1] > 200 && p[0] < 50 && p[2] < 50;

const keyed = project('keyed', [layer({ keys: moving })]);
const f0 = frame(keyed, 0), f30 = frame(keyed, 30);
w.check(green(px(f0, 100, 125)), 'frame 0: the layer in the top-left quarter (its green at 100, 125)', px(f0, 100, 125).join(','));
w.check(red(px(f30, 100, 125)) && red(px(f30, 200, 150)), 'frame 30 (1 s): moved half way — the main red back, the layer\'s red at (200, 150)',
  `${px(f30, 100, 125).join(',')} | ${px(f30, 200, 150).join(',')}`);
const fade = frame(project('fade', [layer({ keys: [[0, 160, 90, 0.5, 100, 0], [2, 160, 90, 0.5, 0, 0]] })]), 30);
const half = px(fade, 100, 125);
w.check(Math.abs(half[0] - 127) <= 4 && Math.abs(half[1] - 127) <= 4, 'opacity keyed 100 → 0: half at 1 s', half.join(','));
const inv = frame(project('invert', [layer({ effects: { invert: true } })]), 0);
w.check(px(inv, 100, 125)[0] > 200 && px(inv, 100, 125)[2] > 200 && px(inv, 100, 125)[1] < 50,
  'Invert on the layer: its green made magenta', px(inv, 100, 125).join(','));
const base = frame(project('base', [layer({})]), 0);
w.check(px(inv, 500, 300).join() === px(base, 500, 300).join(), 'the main video untouched beside it', px(inv, 500, 300).join(','));
const lin = frame(keyed, 15), eased = frame(project('eased', [layer({ keys: [[0, 160, 90, 0.5, 100, 1], [2, 480, 270, 0.5, 100, 0]] })]), 15);
w.check(lin && eased && maxDiff(lin, eased) > 0, 'Ease: at 0.5 s a different place than linear', `max diff ${maxDiff(lin, eased)}`);
const plain = frame(project('plain', [layer({})]), 0);
const noKeys = frame(project('nokeys', [layer({ keys: [], effects: {} })]), 0);
w.check(plain && noKeys && plain.rgba.equals(noKeys.rgba), 'no keys, no effects: byte for byte AV-030\'s', 'equal');
const cpu = frame(keyed, 30, true);
w.check(cpu && maxDiff(f30, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(f30, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'keyed.avloop-30.rgba.png'), path.join(HERE, 'render-keyed-1s.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
