// AV-008's preview check (spec §9.3, S9; §3.7.8): after walk-preview.json — the Live frame 10 the Preview showed
// against `avloop-cli frame` of the same project (mean ≤ 1.0); the Encoded frame 10 close to it (compression only);
// the Encoded temporary file carrying alpha (`alphaOf`: clear corner, solid centre). Then the Preview opened and closed
// five times under --mem-log: the later opens' peaks within 50 MB (Private Bytes) and 32 MB (VRAM) of the first's.
// The project file and the raw frames go when the check is done (the project names the owner's paths).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-previewcheck-'));
const at = (f) => path.join(HERE, f);

// 1. Live = avloop-cli frame 10 of the same project.
const project = at('preview.avloop');
const live = at('live-10.rgba');
if (!fs.existsSync(project) || !fs.existsSync(live)) {
  w.check(false, 'walk-preview.json left its project and frame', 'missing — run walk-preview.json first');
} else {
  const raw = path.join(TMP, 'cli-10.rgba');
  const r = lab.cli(['frame', project, '--at', '10', '-o', path.join(TMP, 'cli-10.png'), '--raw', raw]);
  const ok = r.status === 0 && fs.existsSync(raw);
  const d = ok ? lab.meanDiff(fs.readFileSync(live), fs.readFileSync(raw)) : { mean: NaN, max: NaN };
  w.check(ok && d.mean <= 1.0, 'Live frame 10 = avloop-cli frame 10',
    `${r.out.trim().split(/\r?\n/).pop()} | mean ${d.mean.toFixed(4)}, max ${d.max}`);
}

// 2. Encoded: frame 10 as libvpx gives it back, close to Live's; the file keeps alpha.
const encodedFrame = at('encoded-10.rgba');
const encodedFile = at('encoded.webm');
if (fs.existsSync(encodedFrame) && fs.existsSync(live)) {
  const d = lab.meanDiff(fs.readFileSync(live), fs.readFileSync(encodedFrame));
  w.check(d.mean <= 6, 'Encoded frame 10 against Live (compression only)', `mean ${d.mean.toFixed(3)}, max ${d.max}`);
} else {
  w.check(false, 'Encoded frame 10 written', 'missing');
}
if (fs.existsSync(encodedFile)) {
  const a = lab.alphaOf(encodedFile);
  w.check(a.corner === 0 && a.centre === 255, 'the Encoded file keeps its alpha (libvpx)', JSON.stringify(a));
  const v = lab.videoOf(lab.probe(encodedFile));
  w.note(`encoded: ${v.codec_name} ${v.width}×${v.height} ${v.r_frame_rate}, ${v.nb_frames ?? '?'} frames, ` +
    `${fs.statSync(encodedFile).size} bytes`);
} else {
  w.check(false, 'the Encoded file kept', 'missing');
}
for (const f of [project, live, encodedFrame, encodedFile]) fs.rmSync(f, { force: true });

// 3. Memory: open and close the Preview five times.
const steps = [
  { do: 'open', file: 'samples/counter-640p30.mp4' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
  { do: 'type', on: 'transport.time', text: '1' }, { do: 'key', keys: 'I', on: 'MainWindow' },
  { do: 'type', on: 'transport.time', text: '8.9667' }, { do: 'key', keys: 'O', on: 'MainWindow' },
];
for (let i = 0; i < 6; i++) {
  steps.push(
    { label: `open ${i + 1}` }, { do: 'key', keys: 'F5', on: 'MainWindow' },
    { do: 'waitFor', probe: 'preview.cached', eq: 225, timeout: 60000 }, { do: 'wait', ms: 1500 },
    { do: 'previewClose' }, { do: 'wait', ms: 1500 },
  );
}
const script = path.join(TMP, 'preview-cycle.json');
fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 800, steps }));
const csv = at('memory-preview.csv');
const r = lab.app(['--walk', script, '--walk-out', TMP, '--mem-log', csv]);
w.check(r.status === 0 && /WALK PASSED/.test(r.out), 'the Preview opened and closed six times', `exit ${r.status}`);
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
// The opens after the first against the first: peaks per sixth of the run (one open each).
const total = rows.length ? rows[rows.length - 1][0] : 0;
const sixth = (k) => rows.filter((x) => x[0] >= (total * k) / 6 && x[0] < (total * (k + 1)) / 6);
const peak = (k, col) => Math.max(...sixth(k).map((x) => x[col]));
const peaks = (col) => [0, 1, 2, 3, 4, 5].map((k) => peak(k, col).toFixed(0)).join(' / ');
const growth = (col) => Math.max(...[2, 3, 4, 5].map((k) => peak(k, col))) - peak(1, col);
w.check(growth(1) < 50, 'Private Bytes: opens 3–6 against open 2', `peaks ${peaks(1)} MB → growth ${growth(1).toFixed(1)} MB`);
w.check(growth(2) < 32, 'VRAM: opens 3–6 against open 2', `peaks ${peaks(2)} MB → growth ${growth(2).toFixed(1)} MB`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
