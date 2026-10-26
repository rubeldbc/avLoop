// AV-011's render walk (spec §6.4, S14's keyframes): a white 160×90 clip moved by two keyframes across a 1280×720 crop
// (x 200 → 1000 between 1 s and 3 s), rendered by avloop-cli frame — the export's FrameRenderer — and the clip's centre
// found as the centroid of the output's alpha: Linear halfway (2 s) at x 600; Ease (smoothstep) a quarter of the way
// (1.5 s) at 200 + 800 × 0.15625 = 325; before the first and after the last keyframe it holds; on the GPU and the CPU.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-keys-'));
const clip = path.join(TMP, 'white.mkv');
const made = lab.run(lab.ffmpegExe, ['-v', 'error', '-f', 'lavfi', '-i', 'color=c=white:s=160x90:r=30:d=6', '-c:v', 'ffv1',
  '-pix_fmt', 'yuv420p', clip]);
w.check(made.status === 0, 'a white 160×90 clip', made.err.trim() || '6 s at 30 fps');

function project(name, ease) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 1,
    source: { path: clip.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 4 },
    transform: { x: 200, y: 360, scale: 1, keyframes: [
      { t: 1, x: 200, y: 360, scale: 1, interp: ease ? 'ease' : 'linear' },
      { t: 3, x: 1000, y: 360, scale: 1, interp: 'linear' }] },
    crop: { kind: 'rect', x: 0, y: 0, w: 1280, h: 720 },
    loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  }));
  return file;
}

// The alpha's centroid of output frame k (1280×720 RGBA, premultiplied).
function centre(file, k, cpu) {
  const raw = path.join(TMP, `f${k}.rgba`);
  const r = lab.cli(['frame', file, '--at', String(k), '-o', path.join(TMP, `f${k}.png`), '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  if (r.status !== 0) return null;
  const px = fs.readFileSync(raw);
  let sum = 0;
  let sx = 0;
  let sy = 0;
  for (let i = 3, n = 0; i < px.length; i += 4, n++) {
    const a = px[i];
    if (!a) continue;
    sum += a;
    sx += a * ((n % 1280) + 0.5);
    sy += a * (Math.floor(n / 1280) + 0.5);
  }
  return sum ? { x: sx / sum, y: sy / sum } : null;
}

const linear = project('linear', false);
const eased = project('ease', true);
const cases = [
  ['Linear, halfway (2 s)', linear, 60, 600],
  ['Linear, a quarter (1.5 s)', linear, 45, 400],
  ['Ease, a quarter (1.5 s): smoothstep 0.15625', eased, 45, 325],
  ['Ease, halfway (2 s)', eased, 60, 600],
  ['before the first keyframe (0.5 s): it holds', linear, 15, 200],
  ['after the last keyframe (3.5 s): it holds', linear, 105, 1000],
];
for (const cpu of [false, true]) {
  for (const [what, file, k, x] of cases) {
    const c = centre(file, k, cpu);
    w.check(c && Math.abs(c.x - x) <= 1 && Math.abs(c.y - 360) <= 1, `${what}${cpu ? ' (CPU)' : ''}`,
      c ? `centre ${c.x.toFixed(2)}, ${c.y.toFixed(2)} (want ${x}, 360)` : 'no frame');
  }
}
fs.copyFileSync(path.join(TMP, 'f60.png'), path.join(HERE, 'linear-halfway.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
