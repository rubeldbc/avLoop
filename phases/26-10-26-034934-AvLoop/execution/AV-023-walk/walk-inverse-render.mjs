// AV-023's render walk (D-021): the inverse selection in the export's own frames. The counter sample (640 × 360) with
// an ellipse crop of 200 × 200 centred at (320, 180): not inverted, a 200 × 200 output, its centre opaque and corner
// clear (as AV-004); inverted, a 640 × 360 output with the ellipse a hole — its centre alpha 0, the frame's corners 255,
// the colour outside the hole the video's own; feather 20 rising outward; the GPU and the CPU the same mask; an exported
// WebM keeps the hole (libvpx's alpha); a version-2 file (no "invert") renders as before.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, exportProject } = await import(pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-inverse-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');

function project(name, crop, version = 3) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 },
    loop: { mode: 'simple' },
    crop: { kind: 'ellipse', x: 220, y: 80, w: 200, h: 200, ratio: 'free', radius: 0, feather: 0, ...crop },
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
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);

const own = frame(project('own', {}));
w.check(own && own.w === 200 && own.h === 200, 'not inverted: the ellipse\'s own 200 × 200 output', own ? `${own.w}x${own.h}` : 'no frame');
w.check(px(own, 100, 100)[3] === 255 && px(own, 0, 0)[3] === 0, 'not inverted: centre opaque, corner clear',
  `centre ${px(own, 100, 100)[3]}, corner ${px(own, 0, 0)[3]}`);

const v2 = frame(project('v2', {}, 2));
w.check(v2 && v2.rgba.equals(own.rgba), 'a version-2 file without "invert" renders byte for byte the same', v2 ? `${v2.w}x${v2.h}` : 'no frame');

const hole = frame(project('hole', { invert: true }));
w.check(hole && hole.w === 640 && hole.h === 360, 'inverted: the whole video, 640 × 360', hole ? `${hole.w}x${hole.h}` : 'no frame');
w.check(px(hole, 320, 180)[3] === 0, 'inverted: the hole\'s centre clear', `alpha ${px(hole, 320, 180)[3]}`);
w.check([[0, 0], [639, 0], [0, 359], [639, 359], [100, 180], [540, 180]].every(([x, y]) => px(hole, x, y)[3] === 255),
  'inverted: the corners and both sides opaque', `${px(hole, 0, 0)[3]}, ${px(hole, 639, 359)[3]}, ${px(hole, 100, 180)[3]}`);
w.check(px(hole, 229, 89)[3] === 255, 'the crop\'s own corner (outside the ellipse) kept', `alpha ${px(hole, 229, 89)[3]}`);
const red = px(hole, 40, 250);
w.check(red[0] > 200 && red[1] < 40 && red[2] < 40, 'outside the hole the video\'s own colour (the red bar)', red.join(','));

const soft = frame(project('soft', { invert: true, feather: 20 }));
const ramp = [219, 214, 209, 204, 200].map((x) => px(soft, x, 180)[3]);
w.check(px(soft, 220, 180)[3] <= 13 && ramp.every((a, i) => i === 0 || a >= ramp[i - 1]) && px(soft, 198, 180)[3] === 255,
  'feather 20: alpha rises outward from the edge over ~20 px', `x 220…198: ${px(soft, 220, 180)[3]} → ${ramp.join(' → ')} → ${px(soft, 198, 180)[3]}`);

const cpu = frame(project('soft', { invert: true, feather: 20 }), true);
w.check(cpu && maxDiff(soft, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(soft, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'soft.avloop.rgba.png'), path.join(HERE, 'render-inverse-feather.png'));

const out = path.join(TMP, 'hole.webm');
const r = exportProject(project('hole', { invert: true }), out);
w.check(r.ok && r.size?.[0] === 640, 'exported: VP9 with alpha, 640 × 360', r.text);
if (r.ok) {
  const a = lab.alphaOf(out);
  w.check(a.centre === 0 && a.corner === 255, 'the file\'s alpha (libvpx): the hole clear, the corner opaque',
    `centre ${a.centre}, corner ${a.corner}, mean ${a.mean.toFixed(1)}`);
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
