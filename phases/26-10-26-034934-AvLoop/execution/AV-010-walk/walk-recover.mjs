// AV-010's recovery walk (spec §11.1, §11.3, §12.3), with the app's data folder in a temporary one:
// 1. a change autosaved, then a crash: the process ends, a minidump and the log stay, the autosave stays;
// 2. the next start offers "Recover unsaved project?" — Discard: nothing restored, the autosave gone;
// 3. a change autosaved, then the process killed (taskkill /F) mid-walk;
// 4. the next start offers it again — Recover: the project back as it was, unsaved (`*`); a clean exit leaves no autosave.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawn, spawnSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-recover-'));
const hide = (text) => text.split(TMP).join('…');
const data = path.join(TMP, 'data');
const env = { ...process.env, AVLOOP_DATA_DIR: data };
const autosaves = () => (fs.existsSync(path.join(data, 'autosave'))
  ? fs.readdirSync(path.join(data, 'autosave')).filter((f) => f.endsWith('.avloop')) : []);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const change = (feather) => [
  { do: 'open', file: 'samples/counter-640p30.mp4' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'key', keys: 'E', on: 'MainWindow' }, { do: 'dclick', on: 'canvas', space: 'canvas', at: [320, 180] },
  { do: 'type', on: 'panel.crop.feather', text: String(feather) },
  { do: 'autosaveNow' },
  { do: 'expect', probe: 'autosave.exists', eq: true },
];
function script(name, steps) {
  const file = path.join(TMP, `${name}.json`);
  fs.writeFileSync(file, JSON.stringify({ delay: 120, settle: 700, steps }));
  return ['--walk', file, '--walk-out', TMP];
}
function app(name, steps) {
  const r = lab.app(script(name, steps), { env: { AVLOOP_DATA_DIR: data } });
  const fails = r.out.split(/\r?\n/).filter((l) => l.startsWith('FAIL')).map(hide);
  return { ...r, passed: r.status === 0 && /WALK PASSED/.test(r.out), fails: fails.join(' | ') };
}

// 1. A crash.
const crashed = app('crash', [...change(20), { do: 'crash' }]);
const logs = fs.existsSync(path.join(data, 'logs')) ? fs.readdirSync(path.join(data, 'logs')) : [];
const dumps = logs.filter((f) => f.endsWith('.dmp'));
const log = logs.filter((f) => f.endsWith('.log')).map((f) => fs.readFileSync(path.join(data, 'logs', f), 'utf8')).join('');
w.check(crashed.status !== 0 && !/WALK PASSED/.test(crashed.out), 'a crash ends the process', `exit ${crashed.status}`);
w.check(dumps.length === 1 && fs.statSync(path.join(data, 'logs', dumps[0])).size > 10000, 'a minidump in the logs folder',
  dumps.map((f) => `${f.replace(/\d{8}-\d{6}/, 'YYYYMMDD-HHMMSS')} ${fs.statSync(path.join(data, 'logs', f)).size} bytes`).join(', ') || 'none');
const lines = log.split(/\r?\n/).filter((l) => /\[(app|lab)\]/.test(l));
w.check(lines.some((l) => /\[app\] AvLoop .* started/.test(l)) && lines.some((l) => /\[lab\] error: a crash on purpose/.test(l)),
  'the log: time, module, text', lines.map((l) => l.replace(/^\d\d:\d\d:\d\d\.\d{3}/, 'HH:MM:SS.mmm')).join(' / '));
w.check(autosaves().length === 1, 'the autosave left behind', `${autosaves().length} file(s)`);

// 2. Discard.
const discarded = app('discard', [
  { do: 'answer', choice: 'Discard' }, { do: 'recover' },
  { do: 'expect', probe: 'question.last', contains: 'Recover unsaved project? (autosaved at ' },
  { do: 'expect', probe: 'video.state', eq: 'none' },
]);
w.check(discarded.passed && autosaves().length === 0, 'Discard: nothing restored, the autosave gone',
  discarded.fails || `${autosaves().length} autosave(s) left`);

// 3. Killed.
const child = spawn(lab.bin('avloop'), script('killed', [...change(25), { do: 'wait', ms: 60000 }]),
  { cwd: lab.REPO, env, windowsHide: true, stdio: 'ignore' });
let waited = 0;
while (autosaves().length === 0 && waited < 30000) { await sleep(250); waited += 250; }
await sleep(500);
spawnSync('taskkill', ['/F', '/PID', String(child.pid)]);
await sleep(1000);
w.check(autosaves().length === 1, 'killed after its autosave', `${autosaves().length} file(s) after ${waited} ms`);

// 4. Recover.
const recovered = app('recover', [
  { do: 'recover' },
  { do: 'expect', probe: 'question.last', contains: 'Recover unsaved project?' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'expect', probe: 'crop.kind', eq: 'ellipse' },
  { do: 'expect', probe: 'crop.feather', eq: 25 },
  { do: 'expect', probe: 'project.dirty', eq: true },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop — counter-640p30.mp4*' },
  { do: 'expect', probe: 'autosave.exists', eq: false },
]);
w.check(recovered.passed, 'Recover: the project back, unsaved', recovered.fails || 'Feather 25, the title with *');
w.check(autosaves().length === 0, 'a clean exit leaves no autosave', `${autosaves().length} file(s)`);
w.check(lab.avloopProcesses().length === 0, 'no AvLoop left running', lab.avloopProcesses().join(', ') || 'none');
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
