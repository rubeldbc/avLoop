// AV-009's frame-rate walk (spec §8.3, S8): the 30 fps counter, In 2 s – Out 4 s, Simple, exported at 15, 24, 30, 60 and
// Custom 12 fps with each method (Nearest, Frame blend, Motion): the frame count is D = (Out − In) × fps, ffprobe's
// r_frame_rate the chosen rate, every packet 1/fps after the one before (CFR; WebM keeps milliseconds); Speed 0.5× and
// 2× give twice and half the frames.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { lab, writeProject, exportProject, packets } from './export-lab.mjs';

const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-fps-'));
const video = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const rate = (text) => { const [n, d] = text.split('/').map(Number); return n / (d || 1); };

function exportAt(name, fps, method, speed = 1) {
  const project = writeProject(path.join(TMP, `${name}.avloop`), video,
    { range: { in: 2, out: 4 }, loop: { mode: 'simple', fpsMethod: method, speed }, output: { fps } });
  const out = path.join(TMP, `${name}.webm`);
  return { ...exportProject(project, out), file: out };
}

for (const fps of ['15/1', '24/1', '30/1', '60/1', '12/1']) {
  for (const method of ['nearest', 'blend', 'motion']) {
    const name = `${fps.split('/')[0]}-${method}`;
    const r = exportAt(name, fps, method);
    if (!w.check(r.ok, `${name}: exported`, r.text)) continue;
    const want = Math.round(2 * rate(fps));
    const v = lab.videoOf(lab.probe(r.file));
    const pk = packets(r.file);
    const steps = pk.slice(1).map((p, i) => p.t - pk[i].t);
    const worst = Math.max(...steps.map((s) => Math.abs(s - 1 / rate(fps))));
    w.check(r.frames === want && pk.length === want, `${name}: ${want} frames`, `${r.frames} made, ${pk.length} packets`);
    w.check(Math.abs(rate(v.r_frame_rate) - rate(fps)) < 1e-6, `${name}: r_frame_rate`, v.r_frame_rate);
    w.check(worst <= 0.0011, `${name}: CFR`, `steps ${(1000 / rate(fps)).toFixed(2)} ms ± ${(worst * 1000).toFixed(2)} ms`);
  }
}
for (const speed of [0.5, 2]) {
  const r = exportAt(`speed-${speed}`, '30/1', 'nearest', speed);
  const want = Math.round((2 * 30) / speed);
  w.check(r.ok && r.frames === want, `Speed ${speed}×: ${want} frames`, r.text);
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
