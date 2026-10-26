// AV-007's memory walk (spec §3.7.8): ten minutes' worth of loop frames (18,000 at 30 fps) rendered round a Crossfade
// loop by avloop-cli under --mem-log, on the GPU, then 3,000 on the CPU; the last third's peaks may not rise more
// than 50 MB (Private Bytes) or 32 MB (VRAM) above the first third's.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-memrender-'));
const project = path.join(TMP, 'crossfade.avloop');
fs.writeFileSync(project, JSON.stringify({
  format: 'avloop', version: 1,
  source: { path: path.join(lab.REPO, 'samples', 'counter-640p30.mp4').replace(/\\/g, '/') },
  range: { in: 1.0, out: 6.0 },
  transform: { x: 320, y: 180, scale: 1.0, flipH: false, flipV: false },
  crop: { kind: 'ellipse', x: 170, y: 30, w: 300, h: 300, ratio: '1:1', radius: 0, feather: 8 },
  loop: { mode: 'crossfade', fade: 0.5, curve: 'smooth', turn: 0.3, speed: 1.0, fpsMethod: 'nearest' },
  output: { fps: 'source' },
}, null, 2));

const leg = (name, count, extra) => {
  const csv = path.join(HERE, `memory-render-${name}.csv`);
  const r = lab.cli(['frames', project, '--count', String(count), '--mem-log', csv, ...extra], { timeout: 290000 });
  const line = r.out.trim().split(/\r?\n/).pop();
  w.check(r.status === 0, `${name}: ${count} frames rendered`, line);
  const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
  const third = (k) => rows.slice(Math.floor((rows.length * k) / 3), Math.floor((rows.length * (k + 1)) / 3));
  const peak = (k, col) => Math.max(...third(k).map((x) => x[col]));
  const growth = (col) => peak(2, col) - peak(0, col);
  const peaks = (col) => [0, 1, 2].map((k) => peak(k, col).toFixed(0)).join(' / ');
  w.check(rows.length >= 3 && growth(1) < 50, `${name}: Private Bytes, the last third against the first`,
    `peaks ${peaks(1)} MB → growth ${growth(1).toFixed(1)} MB, ${rows.length} samples`);
  w.check(rows.length >= 3 && growth(2) < 32, `${name}: VRAM`, `peaks ${peaks(2)} MB → growth ${growth(2).toFixed(1)} MB`);
};
leg('gpu', 18000, []);
leg('cpu', 3000, ['--cpu']);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
