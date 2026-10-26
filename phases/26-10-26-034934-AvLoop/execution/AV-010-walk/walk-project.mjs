// AV-010's project walk (spec §11.1, §11.2, S12), in a temporary folder (the files name the owner's paths):
// 1. the app: a loop of a copied clip saved as p1.avloop — the title and its `*` follow the changes, Save, Undo past
//    the save and Redo; Recent; the close question (Cancel stays); New (Don't save); p1 opened again as saved and saved
//    as p2 → avloop-cli frame 10 of p1 and of p2: the same bytes;
// 2. the folder moved: p1 finds its clip by relativePath;
// 3. the clip moved away: p1 says the video was not found (and the log asks for Locate source…); located, it opens.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-project-'));
const hide = (text) => text.split(TMP).join('…'); // the logs keep no path of the owner's
const data = path.join(TMP, 'data');
const A = path.join(TMP, 'a');
fs.mkdirSync(path.join(A, 'src'), { recursive: true });
fs.mkdirSync(path.join(A, 'proj'));
fs.copyFileSync(path.join(lab.REPO, 'samples', 'counter-640p30.mp4'), path.join(A, 'src', 'clip.mp4'));

function app(name, steps, out) {
  const script = path.join(TMP, `${name}.json`);
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 700, steps }));
  const r = lab.app(['--walk', script, '--walk-out', out], { env: { AVLOOP_DATA_DIR: data } });
  const fails = r.out.split(/\r?\n/).filter((l) => l.startsWith('FAIL')).map(hide);
  w.check(r.status === 0 && /WALK PASSED/.test(r.out), `${name}: the app's walk`,
    fails.length ? fails.join(' | ') : `${steps.length} steps passed`);
}

// 1. Save, the title, Undo across the save, Recent, the close question, New, open again, Save As.
const p1 = path.join(A, 'proj', 'p1.avloop');
app('save-open', [
  { do: 'open', file: path.join(A, 'src', 'clip.mp4') },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'type', on: 'transport.time', text: '2' }, { do: 'key', keys: 'I', on: 'MainWindow' },
  { do: 'type', on: 'transport.time', text: '4.9667' }, { do: 'key', keys: 'O', on: 'MainWindow' },
  { do: 'key', keys: 'E', on: 'MainWindow' }, { do: 'dclick', on: 'canvas', space: 'canvas', at: [320, 180] },
  { do: 'type', on: 'panel.crop.feather', text: '12' },
  { do: 'expect', probe: 'project.dirty', eq: true },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop — clip.mp4*' },
  { do: 'saveAs', file: 'p1.avloop' },
  { do: 'expect', probe: 'project.dirty', eq: false },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop — p1.avloop' },
  { do: 'type', on: 'panel.crop.feather', text: '14' },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop — p1.avloop*' },
  { do: 'key', keys: 'Ctrl+S', on: 'MainWindow' },
  { do: 'expect', probe: 'project.dirty', eq: false },
  { do: 'key', keys: 'Ctrl+Z', on: 'MainWindow' },
  { do: 'expect', probe: 'project.dirty', eq: true },
  { do: 'key', keys: 'Ctrl+Y', on: 'MainWindow' },
  { do: 'expect', probe: 'project.dirty', eq: false },
  { do: 'expect', probe: 'recent.projects', contains: 'p1.avloop' },
  { do: 'type', on: 'panel.crop.feather', text: '16' },
  { do: 'answer', choice: 'Cancel' }, { do: 'closeWindow' },
  { do: 'expect', probe: 'question.last', eq: 'Save the changes to p1.avloop?' },
  { do: 'expect', probe: 'project.dirty', eq: true },
  { do: 'answer', choice: "Don't save" }, { do: 'menu', path: ['New'] },
  { do: 'expect', probe: 'video.state', eq: 'none' },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop' },
  { do: 'project', file: p1 },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'expect', probe: 'crop.feather', eq: 14 },
  { do: 'expect', probe: 'crop.kind', eq: 'ellipse' },
  { do: 'expect', probe: 'project.dirty', eq: false },
  { do: 'expect', probe: 'window.title', eq: 'AvLoop — p1.avloop' },
  { do: 'saveAs', file: 'p2.avloop' },
], path.join(A, 'proj'));

// S12: what was saved and what was opened and saved again make the same pixels.
const frame = (project, name) => {
  const raw = path.join(TMP, `${name}.rgba`);
  const r = lab.cli(['frame', project, '--at', '10', '-o', path.join(TMP, `${name}.png`), '--raw', raw]);
  return r.status === 0 && fs.existsSync(raw) ? fs.readFileSync(raw) : null;
};
const f1 = frame(p1, 'p1');
const f2 = frame(path.join(A, 'proj', 'p2.avloop'), 'p2');
const d = f1 && f2 && f1.length === f2.length ? lab.meanDiff(f1, f2) : null;
w.check(d && d.max === 0, 'S12: avloop-cli frame 10 of p1 and of p2 (opened, saved again)',
  d ? `${f1.length} bytes each, mean ${d.mean}, max ${d.max}` : 'no frame');
const j1 = JSON.parse(fs.readFileSync(p1, 'utf8'));
const j2 = JSON.parse(fs.readFileSync(path.join(A, 'proj', 'p2.avloop'), 'utf8'));
delete j1.view; delete j2.view;
w.check(JSON.stringify(j1) === JSON.stringify(j2), 'p2 keeps every key of p1 (the view aside)',
  `${Object.keys(j1).join(', ')}`);

// 2. The folder moved: the clip found beside the project by its relative path.
const B = path.join(TMP, 'b');
fs.renameSync(A, B);
app('moved', [
  { do: 'project', file: path.join(B, 'proj', 'p1.avloop') },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'expect', probe: 'source.file', contains: path.join('b', 'src', 'clip.mp4') },
  { do: 'expect', probe: 'crop.feather', eq: 14 },
], path.join(B, 'proj'));

// 3. The clip moved away: not found, then Locate source… answered.
const C = path.join(TMP, 'c');
fs.mkdirSync(C);
fs.renameSync(path.join(B, 'src', 'clip.mp4'), path.join(C, 'elsewhere.mp4'));
app('locate', [
  { do: 'project', file: path.join(B, 'proj', 'p1.avloop') },
  { do: 'wait', ms: 1000 },
  { do: 'expect', probe: 'video.state', eq: 'none' },
  { do: 'expect', probe: 'video.error', contains: 'was not found' },
  { do: 'answer', text: path.join(C, 'elsewhere.mp4') },
  { do: 'project', file: path.join(B, 'proj', 'p1.avloop') },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
  { do: 'expect', probe: 'source.file', contains: 'elsewhere.mp4' },
  { do: 'expect', probe: 'crop.feather', eq: 14 },
], path.join(B, 'proj'));
const logs = fs.readdirSync(path.join(data, 'logs')).filter((f) => f.endsWith('.log'));
const text = logs.map((f) => fs.readFileSync(path.join(data, 'logs', f), 'utf8')).join('');
const line = text.split(/\r?\n/).find((l) => l.includes('Locate source'));
w.check(!!line, 'the log asks for Locate source…', line ? hide(line).replace(/^\S+ /, '') : `${logs.length} log file(s)`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
