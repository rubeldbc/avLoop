// AV-012's finder walk (spec §11.5, S14): `avloop-cli find` — the app's LoopFinder — on loop4s.mp4 (a picture that
// repeats every 4 s): over the whole file the best loop is 4.000 s (±2 frames); over a project's crop of one of its
// boxes as well; with loops of 2–3 s only (out of the period's reach) every answer keeps to the bounds and scores lower.
// Then the app opening the Finder, finding and closing it four times under --mem-log: after the first round the peaks
// stay within 50 MB and the floor returns (the fingerprints go when the window closes).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-finder-'));
const video = path.join(lab.REPO, 'samples', 'loop4s.mp4');

function find(args) {
  const r = lab.cli(['find', ...args]);
  const loops = [...r.out.matchAll(/^\d\. in ([\d.]+) out ([\d.]+) length ([\d.]+) score (\d+) raw ([\d.]+)/gm)]
    .map((m) => ({ in: Number(m[1]), out: Number(m[2]), length: Number(m[3]), score: Number(m[4]), raw: Number(m[5]) }));
  const head = r.out.split(/\r?\n/).find((l) => l.startsWith('searched')) ?? r.out.trim();
  return { ok: r.status === 0, loops, head };
}

// 1. The whole file.
const whole = find([video]);
w.check(whole.ok && whole.loops.length === 5 && Math.abs(whole.loops[0].length - 4) <= 2 / 30,
  'the whole file: the best loop 4 s', `${whole.head}; best ${JSON.stringify(whole.loops[0])}`);
w.check(whole.loops.every((l, i) => i === 0 || l.raw >= whole.loops[i - 1].raw), 'the five in order of their scores',
  whole.loops.map((l) => `${l.in.toFixed(3)}–${l.out.toFixed(3)} (${l.score})`).join(', '));

// 2. A project's crop around the orange box's lane, In 6 – Out 9 (searched 1 – 14 s).
const project = path.join(TMP, 'crop.avloop');
fs.writeFileSync(project, JSON.stringify({
  format: 'avloop', version: 1, source: { path: video.split(path.sep).join('/'), stream: 0 },
  range: { in: 6, out: 9 }, transform: { x: 320, y: 180, scale: 1 },
  crop: { kind: 'rect', x: 0, y: 120, w: 640, h: 120 }, loop: { mode: 'simple' }, output: { fps: '30/1' },
}));
const cropped = find([project]);
w.check(cropped.ok && Math.abs(cropped.loops[0]?.length - 4) <= 2 / 30, 'a project\'s crop: the best loop 4 s',
  `${cropped.head}; best ${JSON.stringify(cropped.loops[0])}`);

// 3. Lengths out of the period's reach.
const short = find([video, '--min', '2', '--max', '3']);
w.check(short.ok && short.loops.every((l) => l.length >= 2 - 1e-6 && l.length <= 3 + 1e-6) &&
  short.loops[0].raw > whole.loops[0].raw, '2–3 s: the bounds kept, no perfect loop',
  `best ${JSON.stringify(short.loops[0])}`);

// 4. The window four times under --mem-log.
const steps = [
  { do: 'open', file: 'samples/loop4s.mp4' }, { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'wait', ms: 1500 },
];
for (let i = 1; i <= 4; i++)
  steps.push({ label: `round ${i}` }, { do: 'key', keys: 'Ctrl+L', on: 'MainWindow' },
    { do: 'choose', on: 'loopfinder.range', data: 'file' }, { do: 'click', on: 'loopfinder.find' },
    { do: 'waitFor', probe: 'loopfinder.state', eq: 'done', timeout: 60000 }, { do: 'wait', ms: 800 },
    { do: 'finderClose' }, { do: 'wait', ms: 1500 });
steps.push({ label: 'end' });
const script = path.join(TMP, 'finder-cycle.json');
fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 800, steps }));
const csv = path.join(TMP, 'memory.csv');
const app = lab.app(['--walk', script, '--walk-out', TMP, '--mem-log', csv]);
w.check(app.status === 0 && /^WALK PASSED/m.test(app.out), 'the Finder opened, found and closed four times',
  `exit ${app.status}`);
const at = [...app.out.matchAll(/^-- (?:round \d|end) @(\d+) ms/gm)].map((m) => Number(m[1]));
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
const span = (k) => rows.filter((r) => r[0] >= at[k] && r[0] < at[k + 1]);
const peak = (k) => Math.max(...span(k).map((r) => r[1]));
const floorAfter = (k) => Math.min(...span(k).slice(-2).map((r) => r[1]));
const before = Math.min(...rows.filter((r) => r[0] < at[0]).slice(-2).map((r) => r[1]));
w.check(at.length === 5 && Math.max(peak(2), peak(3)) - peak(1) < 50, 'Private Bytes: rounds 3–4 against round 2',
  `peaks ${[0, 1, 2, 3].map(peak).map((v) => v.toFixed(0)).join(' / ')} MB`);
w.check(floorAfter(3) - before < 30, 'closed, the memory comes back',
  `before the first ${before.toFixed(0)} MB; after each close ${[0, 1, 2, 3].map(floorAfter).map((v) => v.toFixed(0)).join(' / ')} MB`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
