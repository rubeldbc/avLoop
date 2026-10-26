// CP-A1's memory walk (spec §3.7.8, S13): one app run under --mem-log — 10 minutes of loop playback of the 1080p
// counter (In 1 s – Out 5 s, an ellipse crop), then 3 exports in a row, then the Preview opened and closed 5 times,
// then 20 s at rest. The run is longer than a command may wait, so it starts detached (`node walk-memory.mjs start`)
// and this walk, run once it has ended, reads its memory log phase by phase (the walk's labels carry the same clock):
// after warm-up — a minute of playback, the first export, the first Preview — Private Bytes grow < 50 MB and VRAM
// < 32 MB: playback's 10th minute against its 2nd, the 3rd export against the 2nd, the later Previews against the
// 2nd, and the app at rest at the end against the pause after the 1st export.
import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const RUN = path.join(lab.REPO, 'build', 'cp-a1-memory'); // build/ is never committed
const csv = path.join(RUN, 'memory.csv');
const done = path.join(RUN, 'exit.txt');
const PLAY_MS = 600000;

if (process.argv[2] === 'start') {
  fs.rmSync(RUN, { recursive: true, force: true });
  fs.mkdirSync(RUN, { recursive: true });
  const steps = [
    { do: 'open', file: 'samples/counter-1080p30.mp4' },
    { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
    { do: 'type', on: 'transport.time', text: '1' }, { do: 'key', keys: 'I', on: 'MainWindow' },
    { do: 'type', on: 'transport.time', text: '4.9667' }, { do: 'key', keys: 'O', on: 'MainWindow' },
    { do: 'key', keys: 'E', on: 'MainWindow' },
    { do: 'drag', on: 'canvas', space: 'canvas', from: [640, 220], to: [1280, 860], steps: 12 },
    { label: 'playback' }, { do: 'key', keys: 'Home', on: 'MainWindow' }, { do: 'key', keys: 'Space', on: 'MainWindow' },
    { do: 'expect', probe: 'playback.playing', eq: true },
    { do: 'wait', ms: PLAY_MS },
    { do: 'expect', probe: 'playback.playing', eq: true },
    { do: 'key', keys: 'Space', on: 'MainWindow' },
  ];
  for (let i = 1; i <= 3; i++)
    steps.push({ label: `export ${i}` }, { do: 'exportTo', file: `export-${i}.webm` },
      { do: 'waitFor', probe: 'export.state', eq: 'done', timeout: 180000 }, { do: 'wait', ms: 3000 });
  for (let i = 1; i <= 5; i++)
    steps.push({ label: `preview ${i}` }, { do: 'key', keys: 'F5', on: 'MainWindow' },
      { do: 'waitFor', probe: 'preview.cached', min: 100, timeout: 60000 }, { do: 'wait', ms: 1000 },
      { do: 'previewClose' }, { do: 'wait', ms: 1500 });
  steps.push({ label: 'idle' }, { do: 'wait', ms: 20000 });
  const script = path.join(RUN, 'memory.json');
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 800, steps }));
  // The app runs on its own; a small cmd marks when it has ended (its verdict is in out.txt).
  const cmd = `"${lab.bin('avloop')}" --walk "${script}" --walk-out "${RUN}" --mem-log "${csv}" > "${path.join(RUN, 'out.txt')}" 2>&1 & echo ended > "${done}"`;
  const child = spawn('cmd.exe', ['/d', '/s', '/c', `"${cmd}"`], { cwd: lab.REPO, detached: true, stdio: 'ignore', windowsHide: true, windowsVerbatimArguments: true });
  child.unref();
  console.log('STARTED the memory run (about 12 minutes)');
  process.exit(0);
}

const w = lab.walkLog();
if (!fs.existsSync(done)) {
  w.check(false, 'the memory run has ended', fs.existsSync(csv) ? 'still running' : 'not started (node walk-memory.mjs start)');
  w.verdict();
  process.exit();
}
const out = fs.readFileSync(path.join(RUN, 'out.txt'), 'utf8');
const failed = out.split(/\r?\n/).filter((l) => l.startsWith('FAIL'));
w.check(/^WALK PASSED/m.test(out), 'the run: playback, 3 exports, 5 Previews, idle',
  failed.length ? failed.join(' | ') : 'every step passed');
// The phases by their labels' times (the --mem-log clock): "-- export 1 @612345 ms".
const at = {};
for (const m of out.matchAll(/^-- (.+) @(\d+) ms/gm)) at[m[1]] = Number(m[2]);
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
const span = (a, b) => rows.filter((r) => r[0] >= a && r[0] < b);
const peak = (set, col) => Math.max(...set.map((r) => r[col]));
const floor = (set, col) => Math.min(...set.map((r) => r[col]));
const mb = (v) => `${v.toFixed(0)} MB`;
function gate(what, base, later, col, limit, kind) {
  const f = kind === 'floor' ? floor : peak;
  const grew = f(later, col) - f(base, col);
  w.check(base.length > 0 && later.length > 0 && grew < limit, what,
    `${kind}s ${mb(f(base, col))} → ${mb(f(later, col))} (${grew >= 0 ? '+' : ''}${grew.toFixed(1)} MB, limit ${limit})`);
}
const play = at.playback;
const minute2 = span(play + 60000, play + 120000);
const minute10 = span(at['export 1'] - 60000, at['export 1'] - 3000);
gate('playback, Private Bytes: the 10th minute against the 2nd', minute2, minute10, 1, 50, 'peak');
gate('playback, VRAM: the 10th minute against the 2nd', minute2, minute10, 2, 32, 'peak');
// Warm-up: the first export and the first Preview make the render service, its context and its thread.
const export2 = span(at['export 2'], at['export 3']);
const export3 = span(at['export 3'], at['preview 1']);
gate('exports, Private Bytes: the 3rd against the 2nd', export2, export3, 1, 50, 'peak');
gate('exports, VRAM: the 3rd against the 2nd', export2, export3, 2, 32, 'peak');
const preview2 = span(at['preview 2'], at['preview 3']);
const previews = span(at['preview 3'], at.idle);
gate('Previews, Private Bytes: the 3rd–5th against the 2nd', preview2, previews, 1, 50, 'peak');
gate('Previews, VRAM: the 3rd–5th against the 2nd', preview2, previews, 2, 32, 'peak');
const afterExport1 = span(at['export 2'] - 3000, at['export 2']);
const idle = span(at.idle + 5000, Infinity);
gate('at rest, Private Bytes: the end against the pause after the 1st export', afterExport1, idle, 1, 50, 'floor');
gate('at rest, VRAM: the end against the pause after the 1st export', afterExport1, idle, 2, 32, 'floor');
w.note(`${rows.length} samples over ${((rows[rows.length - 1][0] - rows[0][0]) / 60000).toFixed(1)} minutes; ` +
  `playback ${mb(floor(minute2, 1))}–${mb(peak(minute10, 1))}; the highest ${mb(peak(rows, 1))}, VRAM ${mb(peak(rows, 2))}; ` +
  `at rest after it all ${mb(floor(idle, 1))}, VRAM ${mb(floor(idle, 2))}`);
fs.copyFileSync(csv, path.join(HERE, 'memory-cp-a1.csv'));
w.verdict();
