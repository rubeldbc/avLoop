// AV-030's render walk (D-027): video layers in the export's own frames. The counter sample (640 × 360) with itself as
// a layer, half size in the top-left quarter, from its 1 s mark: where the layer is, its picture (at x 100 the layer's
// green bar over the main video's red); beside it the main video byte for byte; Opacity 50 % half of each; hidden,
// not yet started or a file without "layers": the main video alone, byte for byte; the GPU and the CPU the same; the
// export carries it; inside a crop's ellipse only.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, exportProject } = await import(pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-layers-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const layer = (extra) => ({ name: 'counter', path: sample, w: 640, h: 360, duration: 10, track: 2, start: 0, in: 1, out: 9,
  x: 160, y: 90, scale: 0.5, opacity: 100, visible: true, locked: false, ...extra });

function project(name, layers, crop) {
  const file = path.join(TMP, `${name}.avloop`);
  const p = {
    format: 'avloop', version: 3, source: { path: sample, stream: 0 }, range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  };
  if (layers) p.layers = layers;
  if (crop) p.crop = crop;
  fs.writeFileSync(file, JSON.stringify(p));
  return file;
}
function frame(file, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', '0', '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  const m = /(\d+)x(\d+)/.exec(r.out);
  return r.status === 0 ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : null;
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);
const same = (a, b) => a && b && a.rgba.equals(b.rgba);

const base = frame(project('base'));
const over = frame(project('over', [layer({})]));
const g = px(base, 100, 125), y = px(over, 100, 125);
w.check(g[0] > 200 && g[1] < 40, 'the main video at (100, 125): the red bar', g.join(','));
w.check(y[1] > 200 && y[0] < 40 && y[2] < 40, 'the layer there: its green bar (its own x 200) over it', y.join(','));
w.check(px(over, 500, 300).every((v, i) => v === px(base, 500, 300)[i]) && px(over, 330, 190).join() === px(base, 330, 190).join(),
  'beside the layer: the main video, as it was', px(over, 500, 300).join(','));
const half = px(frame(project('half', [layer({ opacity: 50 })])), 100, 125);
w.check(Math.abs(half[0] - (g[0] + y[0]) / 2) <= 3 && Math.abs(half[2] - (g[2] + y[2]) / 2) <= 3,
  'Opacity 50 %: half the layer over the main video', half.join(','));
w.check(same(frame(project('hidden', [layer({ visible: false })])), base), 'hidden: the main video alone, byte for byte', 'equal');
w.check(same(frame(project('later', [layer({ start: 5 })])), base), 'not yet started (starts at 5 s): byte for byte', 'equal');
w.check(same(frame(project('v2-like')), base), 'a file without "layers": byte for byte', 'equal');
const cpu = frame(project('over', [layer({})]), true);
w.check(cpu && maxDiff(over, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(over, cpu)}` : 'no frame');
const ell = frame(project('ellipse', [layer({})], { kind: 'ellipse', x: 0, y: 0, w: 640, h: 360, ratio: 'free' }));
w.check(ell && px(ell, 2, 2)[3] === 0 && px(ell, 100, 125).join() === y.join(), 'inside a crop\'s ellipse only: its corner clear, the layer kept',
  ell ? `${px(ell, 2, 2)[3]}; ${px(ell, 100, 125).join(',')}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'over.avloop.rgba.png'), path.join(HERE, 'render-layer.png'));

const out = path.join(TMP, 'over.webm');
const r = exportProject(project('over', [layer({})]), out);
w.check(r.ok && r.frames === 60, 'exported: 60 frames with the layer', r.text);
if (r.ok) {
  const f0 = lab.frameAt(out, 0);
  const at = [...f0.rgba.subarray((125 * f0.w + 100) * 4, (125 * f0.w + 100) * 4 + 4)];
  w.check(at[1] > 180 && at[0] < 70 && at[2] < 70, 'the file\'s first frame: the layer\'s green at (100, 125)', at.join(','));
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
