// AV-005's playback walk (spec §7.3, S3, §3.7.8): 1080p H.264 at 30 and at 60 fps plays 10 s with no frame dropped
// (the clock's counter and the status bar's "Dropped: N"); then a minute of loop playback between In and Out under
// --mem-log, whose last third may not rise more than 50 MB (Private Bytes) or 32 MB (VRAM) above its first.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-play-'));

const walk = (name, steps, extra = []) => {
  const script = path.join(TMP, `${name}.json`);
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 800, steps }));
  const r = lab.app(['--walk', script, '--walk-out', TMP, ...extra]);
  fs.writeFileSync(path.join(HERE, `walk-playback-${name}.walk.txt`), r.out);
  return r;
};

// 1. Ten seconds at 30 and at 60 fps.
const steps = [];
for (const fps of [30, 60]) {
  steps.push(
    { label: `1080p${fps}: Space, 10 s, Space` },
    { do: 'open', file: `samples/counter-1080p${fps}.mp4` },
    { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
    { do: 'wait', ms: 1000 },
    { do: 'expect', probe: 'video.hardware', eq: true },
    { do: 'key', keys: 'Space', on: 'MainWindow' },
    { do: 'wait', ms: 10000 },
    { do: 'key', keys: 'Space', on: 'MainWindow' },
    { do: 'wait', ms: 1200 },
    { do: 'expect', probe: 'playback.dropped', eq: 0 },
    { do: 'expect', probe: 'playback.shown', min: fps * 10 - fps / 2 },
    { do: 'expect', probe: 'status.dropped', eq: 'Dropped: 0' },
  );
}
const ten = walk('ten-seconds', steps);
w.check(ten.status === 0 && /WALK PASSED/.test(ten.out), '10 s at 30 and 60 fps, no frame dropped',
  (ten.out.match(/^(PASS|FAIL) expect playback\.(dropped|shown).*$/gm) ?? []).join(' | '));

// 2. A minute of loop playback between In 2 s and Out 5 s, memory logged.
const csv = path.join(HERE, 'memory-playback.csv');
const minute = walk('minute', [
  { label: 'a minute of loop playback, In 2 s – Out 5 s' },
  { do: 'open', file: 'samples/counter-1080p30.mp4' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
  { do: 'type', on: 'transport.time', text: '2' },
  { do: 'key', keys: 'I', on: 'MainWindow' },
  { do: 'type', on: 'transport.time', text: '4.9667' },
  { do: 'key', keys: 'O', on: 'MainWindow' },
  { do: 'expect', probe: 'range.out', near: [5, 0.001] },
  { do: 'key', keys: 'Space', on: 'MainWindow' },
  { do: 'wait', ms: 60000 },
  { do: 'key', keys: 'Space', on: 'MainWindow' },
  { do: 'wait', ms: 1200 },
  { do: 'expect', probe: 'playback.dropped', eq: 0 },
  { do: 'expect', probe: 'playback.shown', min: 1750 },
], ['--mem-log', csv]);
w.check(minute.status === 0 && /WALK PASSED/.test(minute.out), 'a minute of loop playback ran, no frame dropped',
  (minute.out.match(/^(PASS|FAIL) expect playback\.(dropped|shown).*$/gm) ?? []).join(' | '));

const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
// The minute is the stretch of the log (ms since start) after its first 6 s — opening, In and Out —; its thirds are
// compared.
const start = rows.length ? rows[0][0] + 6000 : 0;
const play = rows.filter((x) => x[0] >= start && x[0] <= start + 60000);
const third = (k) => play.slice(Math.floor((play.length * k) / 3), Math.floor((play.length * (k + 1)) / 3));
const peak = (k, col) => Math.max(...third(k).map((x) => x[col]));
const growth = (col) => peak(2, col) - peak(0, col);
w.check(play.length >= 30 && growth(1) < 50, 'Private Bytes over the minute: the last third against the first',
  `peaks ${[0, 1, 2].map((k) => peak(k, 1).toFixed(0)).join(' / ')} MB → growth ${growth(1).toFixed(1)} MB, ${play.length} samples`);
w.check(play.length >= 30 && growth(2) < 32, 'VRAM over the minute',
  `peaks ${[0, 1, 2].map((k) => peak(k, 2).toFixed(0)).join(' / ')} MB → growth ${growth(2).toFixed(1)} MB`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
