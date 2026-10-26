// AV-009's loop walk (spec §8.2, S7, S11): every loop mode exported from a source whose picture repeats every 2 s (a bar
// crossing the frame), In 1 s – Out 3 s: the frame count is §8.2's L (Simple D, Crossfade D − F, Ping-pong 2D − 2); the
// seam (last → first) differs no more than 1.5 × the median of neighbouring frames; no frame repeats (the Ping-pongs'
// turns included). S11: each file read twice in a row through libvpx (`-stream_loop 1`) gives 2L frames, no error.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { lab, writeProject, exportProject, loopedTwice } from './export-lab.mjs';

const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-loops-'));
const source = path.join(TMP, 'periodic.mkv');
const made = lab.run(lab.ffmpegExe, ['-v', 'error', '-f', 'lavfi', '-i', 'color=c=0x16223A:s=640x360:r=30:d=6',
  '-f', 'lavfi', '-i', 'color=c=0xFF7A1A:s=80x360:r=30:d=6',
  '-filter_complex', "[0][1]overlay=x='mod(t*320\\,640)-40':y=0:shortest=1", '-c:v', 'ffv1', '-pix_fmt', 'yuv420p',
  source]);
w.check(made.status === 0 && fs.existsSync(source), 'a source that repeats every 2 s', made.err.trim() || 'periodic.mkv (FFV1)');

const D = 60; // (3 − 1) × 30
const modes = [
  { mode: 'simple', L: D },
  { mode: 'crossfade', L: D - 15, loop: { fade: 0.5 } },
  { mode: 'pingpong', L: 2 * D - 2 },
  { mode: 'pingpong-smooth', L: 2 * D - 2, loop: { turn: 0.3 } },
  // With alpha: an ellipse as wide as the frame, so the bar is always inside it.
  { mode: 'crossfade', L: D - 15, loop: { fade: 0.5 }, crop: { kind: 'ellipse', x: 0, y: 40, w: 640, h: 280 }, tag: 'alpha' },
];
for (const m of modes) {
  const name = m.tag ? `${m.mode}-${m.tag}` : m.mode;
  const project = writeProject(path.join(TMP, `${name}.avloop`), source,
    { range: { in: 1, out: 3 }, loop: { mode: m.mode, ...m.loop }, crop: m.crop });
  const out = path.join(TMP, `${name}.webm`);
  const r = exportProject(project, out);
  if (!w.check(r.ok, `${name}: exported`, r.text)) continue;
  w.check(r.frames === m.L, `${name}: frame count = L`, `${r.frames} (L = ${m.L})`);
  const s = lab.seamDiff(out);
  w.check(s.frames === m.L, `${name}: frames decoded`, `${s.frames}`);
  w.check(s.ratio <= 1.5, `${name}: seamDiff ≤ 1.5 × the median neighbour difference`,
    `seam ${s.seam.toFixed(2)}, median ${s.median.toFixed(2)}, ratio ${s.ratio.toFixed(2)}`);
  w.check(s.duplicates === 0, `${name}: no repeated frame`, `${s.duplicates} repeats`);
  const twice = loopedTwice(out);
  w.check(twice.frames === 2 * m.L && twice.errors === '', `${name}: read twice in a row (S11)`,
    `${twice.frames} frames (2L = ${2 * m.L})${twice.errors ? `; ${twice.errors}` : ', no error'}`);
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
