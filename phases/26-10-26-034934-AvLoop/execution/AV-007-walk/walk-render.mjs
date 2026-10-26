// AV-007's render walk (spec §3.4, §3.6; AVADR-0001): avloop-cli renders output frame N of every loop mode and of the
// crops, once on the GPU and once with --cpu; the two are compared byte by byte (premultiplied RGBA): mean ≤ 1.0,
// max ≤ 4 — the GPU walk's gate. A few frames are kept as pictures.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-render-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').replace(/\\/g, '/');

const base = {
  format: 'avloop', version: 1, source: { path: sample }, range: { in: 2.0, out: 5.0 },
  transform: { x: 320, y: 180, scale: 1.0, flipH: false, flipV: false },
  crop: { kind: 'rect', x: 0, y: 0, w: 0, h: 0, ratio: 'free', radius: 0, feather: 0 },
  loop: { mode: 'simple', fade: 0.5, curve: 'smooth', turn: 0.3, speed: 1.0, fpsMethod: 'nearest' },
  output: { fps: 'source' },
};
const cases = [
  { name: 'simple', at: 40, set: (p) => { p.loop.mode = 'simple'; } },
  { name: 'crossfade', at: 74, keep: true, set: (p) => { p.loop.mode = 'crossfade'; } },
  { name: 'pingpong', at: 120, set: (p) => { p.loop.mode = 'pingpong'; } },
  { name: 'pingpong-smooth', at: 3, keep: true, set: (p) => { p.loop.mode = 'pingpong-smooth'; } },
  { name: 'rect-crop', at: 10, set: (p) => { p.crop = { ...p.crop, x: 100.5, y: 40.25, w: 300, h: 200 }; } },
  { name: 'ellipse-feather', at: 10, keep: true, set: (p) => {
    p.crop = { ...p.crop, kind: 'ellipse', x: 160, y: 40, w: 280, h: 280, feather: 16 }; p.transform.scale = 1.3;
  } },
  { name: 'rounded-flip', at: 20, set: (p) => {
    p.crop = { ...p.crop, kind: 'rounded', x: 120, y: 60, w: 400, h: 240, radius: 40 }; p.transform.flipH = true;
  } },
  { name: 'frame-blend-half-speed', at: 7, set: (p) => { p.loop.fpsMethod = 'blend'; p.loop.speed = 0.5; } },
  { name: 'motion-60fps', at: 7, keep: true, set: (p) => { p.loop.fpsMethod = 'motion'; p.output.fps = '60/1'; } },
];

for (const c of cases) {
  const project = structuredClone(base);
  c.set(project);
  const file = path.join(TMP, `${c.name}.avloop`);
  fs.writeFileSync(file, JSON.stringify(project, null, 2));
  const outs = {};
  for (const backend of ['gpu', 'cpu']) {
    const png = path.join(TMP, `${c.name}-${backend}.png`);
    const raw = path.join(TMP, `${c.name}-${backend}.rgba`);
    const r = lab.cli(['frame', file, '--at', String(c.at), '-o', png, '--raw', raw, ...(backend === 'cpu' ? ['--cpu'] : [])]);
    const line = r.out.trim().split(/\r?\n/).pop();
    outs[backend] = { ok: r.status === 0 && fs.existsSync(raw), line, raw, png };
  }
  const both = outs.gpu.ok && outs.cpu.ok;
  const d = both ? lab.meanDiff(fs.readFileSync(outs.gpu.raw), fs.readFileSync(outs.cpu.raw)) : { mean: NaN, max: NaN };
  const gpuUsed = /GPU:/.test(outs.gpu.line ?? '');
  w.check(both && gpuUsed && d.mean <= 1.0 && d.max <= 4, `${c.name}: frame ${c.at}, GPU against CPU`,
    `${outs.gpu.line} | CPU agrees: mean ${d.mean.toFixed(4)}, max ${d.max}`);
  if (c.keep && outs.gpu.ok) fs.copyFileSync(outs.gpu.png, path.join(HERE, `render-${c.name}.png`));
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
