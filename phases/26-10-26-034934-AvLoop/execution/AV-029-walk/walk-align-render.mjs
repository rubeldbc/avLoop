// AV-029's render walk (D-026): the layers in the export's own frames. The counter sample, no crop: a Select square
// (100, 80, 200, 200) and a Cut square (200, 80, 200, 200) — the Cut on top clears the overlap; the Cut moved below
// (the order swapped) is undone by the Select over it; the Cut hidden takes no byte out (the Select alone's frame);
// a locked shape renders as any; the GPU and the CPU the same.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-align-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');

function project(name, shapes) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 3,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 }, transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    crop: { kind: 'rect', x: 0, y: 0, w: 0, h: 0, ratio: 'free', shapes },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  }));
  return file;
}
function frame(file, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', '0', '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  const m = /(\d+)x(\d+)/.exec(r.out);
  return r.status === 0 ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : null;
}
const alpha = (f, x, y) => f.rgba[(y * f.w + x) * 4 + 3];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);
const add = { kind: 'rect', role: 'add', x: 100, y: 80, w: 200, h: 200, name: 'A' };
const cut = { kind: 'rect', role: 'cut', x: 200, y: 80, w: 200, h: 200, name: 'B' };
const OVERLAP = [250, 180], A_ONLY = [150, 180];

const top = frame(project('top', [add, cut]));
w.check(alpha(top, ...OVERLAP) === 0 && alpha(top, ...A_ONLY) === 255, 'the Cut on top clears the overlap',
  `overlap ${alpha(top, ...OVERLAP)}, A alone ${alpha(top, ...A_ONLY)}`);
const below = frame(project('below', [cut, add]));
w.check(alpha(below, ...OVERLAP) === 255, 'the order swapped: the Select over the Cut keeps the overlap', `overlap ${alpha(below, ...OVERLAP)}`);
const hidden = frame(project('hidden', [add, { ...cut, visible: false }]));
const alone = frame(project('alone', [add]));
w.check(hidden && alone && hidden.rgba.equals(alone.rgba), 'the Cut hidden: byte for byte the Select alone', hidden ? `overlap ${alpha(hidden, ...OVERLAP)}` : 'no frame');
const locked = frame(project('locked', [add, { ...cut, locked: true }]));
w.check(locked && locked.rgba.equals(top.rgba), 'a locked shape renders as any', locked ? 'equal' : 'no frame');
const cpu = frame(project('top', [add, cut]), true);
w.check(cpu && maxDiff(top, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(top, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'top.avloop.rgba.png'), path.join(HERE, 'render-order.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
