// Closes one phase and promotes the next (EXECUTOR-PREAMBLE §4 steps 4 and 7): the phase's front matter DONE, the next
// eligible phase IN_PROGRESS, AUTONOMOUS_STATE.md's active phase, count and completion record, then PROGRESS.json and the
// index regenerated. It refuses a phase without its execution record.
//   node close-phase.mjs <PHASE> "<one line for the completion record>"
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { PROGRAMME_ROOT, STATE_FILE, TOOLS_DIR, loadPhases, pickNext } from './lib.mjs';

const recordOf = (phase) => `execution/${phase}-EXECUTION.md`;

const [id, line] = process.argv.slice(2);
if (!id || !line) {
  console.log('usage: node close-phase.mjs <PHASE> "<completion line>"');
  process.exit(1);
}
const phases = loadPhases();
const phase = phases.find((p) => p.id === id);
if (!phase) { console.log(`FAIL: no phase ${id}`); process.exit(1); }
if (!fs.existsSync(path.join(PROGRAMME_ROOT, recordOf(id)))) { console.log(`FAIL: ${recordOf(id)} missing`); process.exit(1); }

const setStatus = (p, status) => {
  const text = fs.readFileSync(p.fullPath, 'utf8');
  fs.writeFileSync(p.fullPath, text.replace(/^status: .*$/m, `status: ${status}`));
};
setStatus(phase, 'DONE');

const after = loadPhases();
const next = pickNext(after);
if (next) setStatus(next, 'IN_PROGRESS');
const done = after.filter((p) => p.fm?.status === 'DONE').length;

let state = fs.readFileSync(STATE_FILE, 'utf8').replace(/\r\n/g, '\n');
state = state
  .replace(/^Active phase: .*$/m, `Active phase: ${next?.id ?? 'none'}`)
  .replace(/^Next eligible: .*$/m, `Next eligible: ${next ? `${next.id} (in progress)` : 'none — every phase is closed'}`)
  .replace(/^Completed: \d+ of (\d+)$/m, (_, total) => `Completed: ${done} of ${total}`);
const marker = '## Completion record\n';
const at = state.indexOf(marker);
if (at < 0) { console.log('FAIL: no completion record section'); process.exit(1); }
const body = state.slice(at + marker.length);
const today = new Date().toLocaleDateString('sv-SE');
const entry = `- **${id}** DONE ${today} — ${line} Record: \`${recordOf(id)}\`.\n`;
const cleaned = body.replace(/\n- \(none yet — the programme is planned\)\n?/, '\n');
const insertAt = cleaned.indexOf('\n- ') >= 0 ? cleaned.indexOf('\n- ') + 1 : cleaned.length;
state = state.slice(0, at + marker.length) + cleaned.slice(0, insertAt) + entry + cleaned.slice(insertAt);
fs.writeFileSync(STATE_FILE, state);

const run = (tool, args = []) => execFileSync(process.execPath, [path.join(TOOLS_DIR, tool), ...args], { encoding: 'utf8' }).trim();
console.log(run('sync-progress.mjs'));
console.log(run('validate-phase-system.mjs'));
console.log(run('check-continuity.mjs'));
console.log(`${id} DONE; next ${next?.id ?? 'none'}`);
