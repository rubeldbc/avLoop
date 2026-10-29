// AV-025's render walk (D-022): a Pen path in the export's own frames. The counter sample, no crop: a square path
// (100, 100) – (300, 300) whose top segment bulges up by its two handles (to y ≈ 40) — a pixel between the chord and the
// curve is inside, one above the curve outside; the same anchors as a polygon (no handles) leave that pixel clear; the
// GPU and the CPU the same; a file of version 3 with handles read back as written.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-pen-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');

function project(name, shape) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 3,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 }, transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    crop: { kind: 'rect', x: 0, y: 0, w: 0, h: 0, ratio: 'free', shapes: [shape] },
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

// [x, y, in x, in y, out x, out y, smooth]: anchor 0's out and anchor 1's in pull the top up.
const curved = { kind: 'path', role: 'add', points: [[100, 100, 100, 100, 150, 20, 0], [300, 100, 250, 20, 300, 100, 0], [300, 300], [100, 300]] };
const flat = { kind: 'polygon', role: 'add', points: [[100, 100], [300, 100], [300, 300], [100, 300]] };

const c = frame(project('curved', curved));
w.check(c && c.w === 640, 'the path renders, 640 × 360', c ? `${c.w}x${c.h}` : 'no frame');
w.check(alpha(c, 200, 70) === 255, 'between the chord (y 100) and the curve (y ≈ 40): inside', `alpha ${alpha(c, 200, 70)}`);
w.check(alpha(c, 200, 30) === 0, 'above the curve: outside', `alpha ${alpha(c, 200, 30)}`);
w.check(alpha(c, 200, 200) === 255 && alpha(c, 50, 200) === 0 && alpha(c, 350, 200) === 0,
  'the square below: inside; beside it: outside', `${alpha(c, 200, 200)}, ${alpha(c, 50, 200)}, ${alpha(c, 350, 200)}`);
const top = [];
for (let y = 30; y <= 60; y += 2) top.push(alpha(c, 200, y));
const edgeAt = 30 + 2 * top.findIndex((a) => a > 127);
w.check(edgeAt >= 36 && edgeAt <= 46, 'the curve\'s top at x 200 where the cubic says (y = 40)', `alpha crosses ½ at y ≈ ${edgeAt}`);

const f = frame(project('flat', flat));
w.check(alpha(f, 200, 70) === 0 && alpha(f, 200, 200) === 255, 'the same anchors without handles: the chord, that pixel clear',
  `${alpha(f, 200, 70)}, ${alpha(f, 200, 200)}`);

const cpu = frame(project('curved', curved), true);
w.check(cpu && maxDiff(c, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(c, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'curved.avloop.rgba.png'), path.join(HERE, 'render-pen.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
