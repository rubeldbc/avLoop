// AV-032's sweep (the owner's item 28, as CP-A1 and CP-B1 did): every walk of every phase run again on one fresh
// build (`build\bin`), one at a time through walk-phase.mjs (each under five minutes, nothing left behind); each
// walk's log copied to sweep/<phase>-<walk>.log, a line a walk in sweep/walk-phase.log; the earlier folders put back
// as their phases left them. Skipped, and why: the install and release walks (R-75: they would take over the owner's
// own installed AvLoop or rebuild its released installer).
//   node sweep.mjs [from-index]
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const PROGRAMME = path.resolve(HERE, '..', '..');
const EXECUTION = path.join(PROGRAMME, 'execution');
const OUT = path.join(HERE, 'sweep');
fs.mkdirSync(OUT, { recursive: true });
const SKIP = { 'AV-014-walk/walk-install.mjs': 'R-75', 'AV-017-walk/walk-name.mjs': 'R-75', 'AV-021-walk/walk-release.mjs': 'R-75' };

const walks = [];
for (const folder of fs.readdirSync(EXECUTION).filter((d) => /^AV-0(0|1|2)\d-walk$|^AV-03[01]-walk$/.test(d)).sort())
  for (const f of fs.readdirSync(path.join(EXECUTION, folder)).filter((f) => /^walk-.*\.(json|mjs)$/.test(f)).sort())
    walks.push([folder, f]);
const from = Number(process.argv[2] ?? 0);
const log = (line) => { console.log(line); fs.appendFileSync(path.join(OUT, 'walk-phase.log'), `${line}\n`); };
let failed = 0;
walks.forEach(([folder, walk], i) => {
  if (i < from) return;
  const key = `${folder}/${walk}`;
  if (SKIP[key]) { log(`SKIPPED ${key} (${SKIP[key]})`); return; }
  const began = Date.now();
  const r = spawnSync(process.execPath, [path.join(PROGRAMME, 'control', 'lab', 'walk-phase.mjs'), `execution/${folder}`, walk],
    { cwd: PROGRAMME, encoding: 'utf8', timeout: 298000 });
  const said = `${r.stdout ?? ''}`.trim().split(/\r?\n/);
  const verdict = said.find((l) => /^(PASSED|FAILED|MISSING)/.test(l)) ?? `NO VERDICT (${r.status ?? r.signal})`;
  const fails = said.filter((l) => /^\s+FAIL/.test(l)).slice(0, 4);
  log(`[${i}] ${folder.slice(0, 6)} ${verdict.replace(/\(\d+ s\)/, `(${Math.round((Date.now() - began) / 1000)} s)`)}`);
  for (const f of fails) log(`      ${f.trim()}`);
  if (!/^PASSED/.test(verdict)) failed++;
  const walkLog = path.join(EXECUTION, folder, `${walk}.log`);
  if (fs.existsSync(walkLog)) fs.copyFileSync(walkLog, path.join(OUT, `${folder.slice(0, 6)}-${walk}.log`));
});
// The earlier folders as their phases left them (pictures and logs the walks wrote again).
const tracked = walks.map(([f]) => `phases/26-10-26-034934-AvLoop/execution/${f}`).filter((v, i, a) => a.indexOf(v) === i);
spawnSync('git', ['checkout', '--', ...tracked], { cwd: path.resolve(PROGRAMME, '..', '..') });
spawnSync('git', ['clean', '-fdq', '--', ...tracked], { cwd: path.resolve(PROGRAMME, '..', '..') });
log(failed ? `SWEEP: ${failed} walk(s) not PASSED` : 'SWEEP PASSED');
process.exitCode = failed ? 1 : 0;
