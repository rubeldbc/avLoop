// AV-002's memory walk (spec §3.7.8, the decode leg): the window opens four videos in turn, four rounds, under
// --mem-log; each new video releases everything of the previous one, so after warm-up the later rounds' peaks stay
// within 50 MB (Private Bytes) and 32 MB (VRAM) of the second round's. The first round is the warm-up: each path (the
// GPU decoder, HDR, alpha) is made for the first time in it, after the 4K open that sets its peak (mended at CP-A1;
// the rounds are cut by the walk's timed labels). (test-memory holds the same for 600 random seeks.)
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-mem-'));
const files = ['samples/counter-1080p30.mp4', 'samples/uhd-4k.mp4', 'samples/hdr-pq.mkv', 'samples/alpha-vp9.webm'];
const ROUNDS = 4;
const steps = [];
for (let round = 0; round < ROUNDS; round++) {
  steps.push({ label: `round ${round + 1}` });
  for (const f of files) {
    steps.push({ do: 'open', file: f }, { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 }, { do: 'wait', ms: 1200 });
  }
  steps.push({ do: 'wait', ms: 1500 });
}
steps.push({ label: 'end' });
const script = path.join(TMP, 'memory-cycle.json');
fs.writeFileSync(script, JSON.stringify({ delay: 100, settle: 800, steps }));
const csv = path.join(HERE, 'memory-decode.csv');
const r = lab.app(['--walk', script, '--walk-out', TMP, '--mem-log', csv]);
w.check(r.status === 0 && /^WALK PASSED/m.test(r.out), 'the open cycle ran', `${steps.length} steps, exit ${r.status}`);

const at = [...r.out.matchAll(/^-- (?:round \d+|end) @(\d+) ms/gm)].map((m) => Number(m[1]));
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
const round = (k) => rows.filter((x) => x[0] >= at[k] && x[0] < at[k + 1]);
const peak = (k, col) => Math.max(...round(k).map((x) => x[col]));
const growth = (col) => Math.max(...[2, 3].map((k) => peak(k, col))) - peak(1, col);
const peaks = (col) => [0, 1, 2, 3].map((k) => peak(k, col).toFixed(0)).join(' / ');
w.check(at.length === ROUNDS + 1, 'the rounds found by their labels', `${at.length - 1} rounds`);
w.check(growth(1) < 50, 'Private Bytes: rounds 3–4 against round 2 (round 1 the warm-up)',
  `round peaks ${peaks(1)} MB → growth ${growth(1).toFixed(1)} MB, ${rows.length} samples`);
w.check(growth(2) < 32, 'VRAM: rounds 3–4 against round 2',
  `round peaks ${peaks(2)} MB → growth ${growth(2).toFixed(1)} MB`);
w.note(`cache at the end: ${rows.length ? rows[rows.length - 1][3].toFixed(0) : '?'} MB`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
