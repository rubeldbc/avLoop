// AV-026's render walk (D-023): the combinations in the export's own frames. The counter sample, no crop: a Select
// square A (100, 80, 200, 200) with B (200, 80, 200, 200) over it — B Keep only: only the overlap; B Exclude: the two
// sides, not the overlap; a combined shape as the file writes it (two outlines — a ring: an outer square and a hole):
// the band opaque, the hole clear; the GPU and the CPU the same.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-combine-'));
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
const sq = (role, x) => ({ kind: 'rect', role, x, y: 80, w: 200, h: 200 });
const A = [150, 180], BOTH = [250, 180], B = [350, 180], NONE = [500, 180];
const read = (f) => [A, BOTH, B, NONE].map(([x, y]) => alpha(f, x, y)).join(', ');

const keep = frame(project('keep', [sq('add', 100), sq('keep', 200)]));
w.check(read(keep) === '0, 255, 0, 0', 'B Keep only: only where A and B meet', `A, both, B, neither: ${read(keep)}`);
const excl = frame(project('exclude', [sq('add', 100), sq('exclude', 200)]));
w.check(read(excl) === '255, 0, 255, 0', 'B Exclude: where one is, not both', `A, both, B, neither: ${read(excl)}`);
const cutBelow = frame(project('order', [sq('cut', 100), sq('add', 100)]));
w.check(alpha(cutBelow, ...A) === 255, 'the order matters: an Add above a Cut keeps its inside', `alpha ${alpha(cutBelow, ...A)}`);

// A combined ring as the file writes it: the outer square, then the hole's outline starting with the eighth value 1.
const ring = { kind: 'path', role: 'add', points: [[100, 50], [400, 50], [400, 330], [100, 330],
  [200, 140, 200, 140, 200, 140, 0, 1], [300, 140], [300, 240], [200, 240]] };
const r = frame(project('ring', [ring]));
w.check(alpha(r, 120, 70) === 255 && alpha(r, 380, 310) === 255, 'the ring\'s band opaque', `${alpha(r, 120, 70)}, ${alpha(r, 380, 310)}`);
w.check(alpha(r, 250, 190) === 0, 'its hole clear (two outlines, odd-even)', `alpha ${alpha(r, 250, 190)}`);
w.check(alpha(r, 50, 190) === 0, 'outside clear', `alpha ${alpha(r, 50, 190)}`);

const cpu = frame(project('exclude', [sq('add', 100), sq('exclude', 200)]), true);
w.check(cpu && maxDiff(excl, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(excl, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'ring.avloop.rgba.png'), path.join(HERE, 'render-ring.png'));
fs.copyFileSync(path.join(TMP, 'exclude.avloop.rgba.png'), path.join(HERE, 'render-exclude.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
