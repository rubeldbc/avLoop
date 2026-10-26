// A phase's walks in one run (control/04 §3): the built binaries looked for, the phase's own walks run one by one —
// a `.json` by `avloop.exe --walk <script> --walk-out <folder>` (real widgets, real clicks, screenshots), a `.mjs` by its
// own Node process (avloop-cli, ffprobe, av-lab.mjs readings) —, each under five minutes; then the check that nothing
// of the walk is left: no avloop process, no session cache folder, no lab folder, no temp file of the walk. A line a
// walk, kept in walk-phase.log. Node built-ins only.
//   node walk-phase.mjs <the phase's walk folder> <walk.json | walk.mjs …>
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.join(HERE, 'av-lab.mjs')).href);

const [folder, ...walks] = process.argv.slice(2);
if (!folder || walks.length === 0) { console.log('usage: node walk-phase.mjs <folder> <walk.json | walk.mjs …>'); process.exit(1); }
const into = path.resolve(folder);
fs.mkdirSync(into, { recursive: true });
const said = [];
const line = (text) => { console.log(text); said.push(text); };

for (const exe of ['avloop', 'avloop-cli']) {
  if (!fs.existsSync(lab.bin(exe))) { line(`NOT BUILT: ${path.relative(lab.REPO, lab.bin(exe))} — run scripts\\build.ps1 first`); process.exit(2); }
}
const before = new Set(lab.cacheSessions());
// A lab run's own settings, autosave and logs folder: removed when the app ends (AV-012 found them left).
const labFolders = () => fs.readdirSync(os.tmpdir()).filter((f) => f.startsWith('avloop-lab-'));
const labBefore = new Set(labFolders());
let failed = 0;
for (const walk of walks) {
  const script = path.resolve(into, walk);
  if (!fs.existsSync(script)) { line(`MISSING ${walk}`); failed++; continue; }
  const began = Date.now();
  const run = walk.endsWith('.json')
    ? spawnSync(lab.bin('avloop'), ['--walk', script, '--walk-out', into], { encoding: 'utf8', cwd: lab.REPO, timeout: 295000 })
    : spawnSync(process.execPath, [script], { encoding: 'utf8', cwd: into, timeout: 295000 });
  const out = `${run.stdout ?? ''}${run.stderr ?? ''}`;
  fs.writeFileSync(path.join(into, `${path.basename(walk)}.log`), out);
  // The verdict is a line of its own (a check's text may quote another walk's), and the exit code must agree.
  const verdict = /^WALK PASSED\s*$/m.test(out) && run.status === 0 ? 'PASSED'
    : /^WALK FAILED/m.test(out) ? 'FAILED' : `ENDED ${run.status ?? run.signal}`;
  if (verdict !== 'PASSED') failed++;
  line(`${verdict} ${walk} (${Math.round((Date.now() - began) / 1000)} s)`);
  for (const bad of out.split(/\r?\n/).filter((l) => /^FAIL /.test(l))) line(`   ${bad}`);
}

const left = lab.avloopProcesses();
if (left.length) { line(`FAIL a process of the walk left: ${left.join(', ')}`); failed++; }
const sessions = lab.cacheSessions().filter((s) => !before.has(s));
if (sessions.length) { line(`FAIL a session cache left: ${sessions.join(', ')}`); failed++; }
const labs = labFolders().filter((f) => !labBefore.has(f));
if (labs.length) { line(`FAIL a lab folder left in the temp folder: ${labs.join(', ')}`); failed++; }
const temps = fs.readdirSync(into).filter((f) => /\.(part|tmp)$/i.test(f));
if (temps.length) { line(`FAIL a temp file of the walk left: ${temps.join(', ')}`); failed++; }

line(failed ? `PHASE WALKS FAILED: ${failed}` : 'PHASE WALKS PASSED');
fs.writeFileSync(path.join(into, 'walk-phase.log'), `${said.join('\n')}\n`);
process.exit(failed ? 1 : 0);
