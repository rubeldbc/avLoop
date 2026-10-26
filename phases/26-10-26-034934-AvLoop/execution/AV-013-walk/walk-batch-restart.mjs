// AV-013's restart walk (spec §10.5), with the app's data folder (and so queue.json) in a temporary one:
// 1. two entries added, Start, and a crash while the first runs: the process ends with the queue saved;
// 2. the next start shows both as Pending (the one under way starts over), the window pictured; Start finishes both,
//    and no `.part` is left.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-queue-'));
const hide = (text) => text.split(TMP).join('…');
const data = path.join(TMP, 'data');
// A crashed app cannot remove its lab folder (settings, logs): this walk removes the ones its runs left.
const labFolders = () => fs.readdirSync(os.tmpdir()).filter((f) => f.startsWith('avloop-lab-'));
const labBefore = new Set(labFolders());

function app(name, steps) {
  const file = path.join(TMP, `${name}.json`);
  fs.writeFileSync(file, JSON.stringify({ delay: 120, settle: 700, steps }));
  const r = lab.app(['--walk', file, '--walk-out', TMP], { env: { AVLOOP_DATA_DIR: data } });
  const fails = r.out.split(/\r?\n/).filter((l) => l.startsWith('FAIL')).map(hide);
  return { ...r, passed: r.status === 0 && /^WALK PASSED\s*$/m.test(r.out), fails: fails.join(' | ') };
}
const queue = () => {
  const f = path.join(data, 'queue.json');
  return fs.existsSync(f) ? JSON.parse(fs.readFileSync(f, 'utf8')).entries.map((e) => e.state) : [];
};

// 1. Two ProRes entries (each a few seconds), Start, a crash while the first runs.
const crashed = app('crash', [
  { do: 'open', file: 'samples/counter-640p30.mp4' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'choose', on: 'panel.output.format', data: 'mov-prores' },
  { do: 'batchAdd', file: 'first.mov', temp: true },
  { do: 'batchAdd', file: 'second.mov', temp: true },
  { do: 'batchStart' },
  { do: 'waitFor', probe: 'batch.states', eq: 'running,pending', timeout: 10000 },
  { do: 'crash' },
]);
w.check(crashed.status !== 0 && !/^WALK PASSED/m.test(crashed.out), 'a crash while the first entry runs',
  `exit ${crashed.status}`);
w.check(queue().join(',') === 'running,pending', 'queue.json as the crash left it', queue().join(',') || 'no file');

// 2. Reopened: both Pending; Start finishes them.
const reopened = app('reopen', [
  { do: 'expect', probe: 'batch.count', eq: 2 },
  { do: 'expect', probe: 'batch.states', eq: 'pending,pending' },
  { do: 'expect', probe: 'batch.files', eq: 'first.mov,second.mov' },
  { do: 'menu', path: ['Batch Queue…'] },
  { do: 'shot', on: 'BatchWindow', file: 'batch-restart.png' },
  { do: 'batchStart' },
  { do: 'waitFor', probe: 'batch.states', eq: 'done,done', timeout: 120000 },
  { do: 'batchClose' },
]);
w.check(reopened.passed, 'reopened: both Pending, then both Done', reopened.fails || 'pending,pending → done,done');
if (fs.existsSync(path.join(TMP, 'batch-restart.png'))) fs.copyFileSync(path.join(TMP, 'batch-restart.png'), path.join(HERE, 'batch-restart.png'));
const written = fs.existsSync(data) ? fs.readdirSync(data).filter((f) => /\.mov(\.part)?$/.test(f)) : [];
w.check(written.join(',') === 'first.mov,second.mov' && written.every((f) => fs.statSync(path.join(data, f)).size > 1e6),
  'both files written, no .part left', written.map((f) => `${f} ${fs.statSync(path.join(data, f)).size} bytes`).join(', '));
w.check(lab.avloopProcesses().length === 0, 'no AvLoop left running', lab.avloopProcesses().join(', ') || 'none');

for (const f of labFolders().filter((x) => !labBefore.has(x))) fs.rmSync(path.join(os.tmpdir(), f), { recursive: true, force: true });
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
