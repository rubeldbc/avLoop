// AV-024's render walk (D-022): shapes in the export's own frames. The counter sample (640 × 360), no crop: a Select
// rectangle with a Cut ellipse in it — inside the rectangle opaque, inside the ellipse clear, outside both clear; cuts
// alone (a Cut rectangle) clear only their inside; a polygon's inside and outside; with a crop the shapes inside its
// frame; the GPU and the CPU the same; an export keeps them (libvpx's alpha); no shapes, AV-023's frames.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, exportProject } = await import(pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-shapes-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');

const rect = (role, x, y, ww, hh, kind = 'rect') => ({ kind, role, x, y, w: ww, h: hh });
function project(name, shapes, crop = {}) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 3,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 },
    loop: { mode: 'simple' },
    crop: { kind: 'rect', x: 0, y: 0, w: 0, h: 0, ratio: 'free', ...crop, shapes },
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

const two = [rect('add', 100, 60, 300, 200), rect('cut', 200, 110, 100, 100, 'ellipse')];
const f = frame(project('two', two));
w.check(f && f.w === 640 && f.h === 360, 'no crop: the whole video, 640 × 360', f ? `${f.w}x${f.h}` : 'no frame');
w.check(alpha(f, 120, 80) === 255 && alpha(f, 380, 240) === 255, 'inside the Select rectangle opaque',
  `${alpha(f, 120, 80)}, ${alpha(f, 380, 240)}`);
w.check(alpha(f, 250, 160) === 0, 'inside the Cut ellipse clear', `alpha ${alpha(f, 250, 160)}`);
w.check(alpha(f, 205, 115) === 255, 'the ellipse\'s box corner (outside it) opaque', `alpha ${alpha(f, 205, 115)}`);
w.check([[10, 10], [630, 350], [50, 200], [500, 100]].every(([x, y]) => alpha(f, x, y) === 0),
  'outside the shapes clear', `${alpha(f, 10, 10)}, ${alpha(f, 630, 350)}`);
const edge = [98, 99, 100, 101].map((x) => alpha(f, x, 150));
w.check(edge[0] === 0 && edge[3] === 255, 'the rectangle\'s edge anti-aliased within a pixel', edge.join(' → '));

const cut = frame(project('cut', [rect('cut', 200, 100, 200, 100)]));
w.check(alpha(cut, 300, 150) === 0 && alpha(cut, 10, 10) === 255 && alpha(cut, 630, 350) === 255,
  'a Cut alone: its inside clear, the rest kept', `${alpha(cut, 300, 150)}, ${alpha(cut, 10, 10)}`);

const tri = { kind: 'polygon', role: 'add', points: [[100, 50], [400, 50], [100, 300]] };
const t = frame(project('tri', [tri]));
w.check(alpha(t, 150, 100) === 255 && alpha(t, 350, 250) === 0, 'a polygon: its inside opaque, beyond its slope clear',
  `${alpha(t, 150, 100)}, ${alpha(t, 350, 250)}`);

const cropped = frame(project('cropped', [rect('add', 0, 0, 100, 100)], { x: 50, y: 50, w: 200, h: 200 }));
w.check(cropped && cropped.w === 200 && alpha(cropped, 20, 20) === 255 && alpha(cropped, 100, 100) === 0,
  'with a crop: the shapes in its frame (the rectangle\'s part inside it)', cropped ? `${cropped.w}x${cropped.h}, ${alpha(cropped, 20, 20)}, ${alpha(cropped, 100, 100)}` : 'no frame');

const cpu = frame(project('two', two), true);
w.check(cpu && maxDiff(f, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(f, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'two.avloop.rgba.png'), path.join(HERE, 'render-shapes.png'));

const none = frame(project('none', [], { kind: 'ellipse', x: 220, y: 80, w: 200, h: 200 }));
w.check(none && none.w === 200 && alpha(none, 100, 100) === 255 && alpha(none, 0, 0) === 0,
  'no shapes: the crop\'s own mask as before', none ? `${none.w}x${none.h}` : 'no frame');

const out = path.join(TMP, 'two.webm');
const r = exportProject(project('two', two), out);
w.check(r.ok, 'exported with alpha', r.text);
if (r.ok) {
  const a = lab.frameAt(out, 0);
  const at = (x, y) => a.rgba[(y * a.w + x) * 4 + 3];
  w.check(at(120, 80) > 240 && at(250, 160) < 15 && at(10, 10) < 15, 'the file\'s alpha (libvpx): the rectangle, the hole, the outside',
    `${at(120, 80)}, ${at(250, 160)}, ${at(10, 10)}`);
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
