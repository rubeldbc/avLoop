// AV-031's memory walk (D-027, spec §3.7.8, AVADR-0002): a loop of the counter with three video layers on V2 … V4,
// rendered round and round by avloop-cli frames — 600 frames with the memory logged each second: after the first
// second (the decoders opened), Private Bytes grow less than 50 MB; every frame made.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-tracks-mem-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const layer = (track, x, start) => ({ name: `v${track}`, path: sample, w: 640, h: 360, duration: 10, track, start, in: 1,
  out: 9, x, y: 180, scale: 0.4, opacity: 80, visible: true, locked: false });
const project = path.join(TMP, 'three.avloop');
fs.writeFileSync(project, JSON.stringify({
  format: 'avloop', version: 3, source: { path: sample, stream: 0 }, range: { in: 0, out: 4 },
  transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'crossfade', fade: 0.5 },
  output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  layers: [layer(2, 140, 0), layer(3, 320, 0.5), layer(4, 500, 1)],
}));
const csv = path.join(TMP, 'memory.csv');
const r = lab.cli(['frames', project, '--count', '600', '--mem-log', csv]);
w.check(r.status === 0, 'avloop-cli frames: 600 frames of a loop with three layers', r.out.trim().split(/\r?\n/).pop());
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
w.check(rows.length >= 3, 'the memory logged each second', `${rows.length} samples`);
if (rows.length >= 3) {
  const after = rows[1][1];
  const peak = Math.max(...rows.slice(1).map((x) => x[1]));
  const last = rows[rows.length - 1][1];
  w.check(peak - after < 50, 'Private Bytes after the first second: growth under 50 MB',
    `${after.toFixed(1)} → peak ${peak.toFixed(1)}, last ${last.toFixed(1)} MB (+${(peak - after).toFixed(1)})`);
}
fs.copyFileSync(csv, path.join(HERE, 'memory-tracks.csv'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
