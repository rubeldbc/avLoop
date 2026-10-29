// Validates the AVSTUDIO phase system. Exit 0 = PASS, 1 = FAIL.
// The example programme's validator with this programme's prefix: phase files (id, ordinal = owner item, headings,
// resume pointer, H1, 300 lines), read_scope (an existing path, `spec: §…`, or `after AS-NNN: <path>` for a file a
// dependency creates), the graph, coverage of the owner's items, a late phase's own rules, PROGRESS.json and AUTONOMOUS_STATE.md.
import fs from 'node:fs';
import path from 'node:path';
import {
  PROGRAMME_ROOT, EXECUTION_DIR, STATE_FILE, OWNER_ITEMS, STATUSES, ACTIVE_STATUSES, RISKS, PERMISSION_CLASSES, TRACKS,
  REQUIRED_HEADINGS, PHASE_ID_RE, PHASE_PREFIX, loadPhases, loadRequirementIds, walkFiles, countLines, checkAcyclic,
  readProgress, isEligible, trackRoots, TERMINAL_OK, REPO_ROOT, PROGRAM_ID, pickNext, ordinalOf,
} from './lib.mjs';

const errors = [];
const warnings = [];
const fail = (m) => errors.push(m);
const warn = (m) => warnings.push(m);

const phases = loadPhases();
const byId = new Map(phases.map((p) => [p.id, p]));
const reqIds = loadRequirementIds();
const roots = trackRoots(phases.filter((p) => p.fm));
const trackRe = new RegExp(`^[${Object.keys(TRACKS).join('')}]$`);
const cpRe = new RegExp(`^CP-[${Object.keys(TRACKS).join('')}]1$`);
const afterRe = new RegExp(`^after (${PHASE_PREFIX}-\\d{3}[a-z]?): (.+)$`);

/** Every direct and transitive dependency of a phase (the read_scope rule: `after AS-NNN: <path>` names one of them). */
function transitiveDeps(id) {
  const seen = new Set();
  const stack = [...(byId.get(id)?.fm?.depends_on ?? [])];
  while (stack.length) {
    const d = stack.pop();
    if (seen.has(d)) continue;
    seen.add(d);
    stack.push(...(byId.get(d)?.fm?.depends_on ?? []));
  }
  return seen;
}

if (phases.length === 0) fail('no phase files found');
if (byId.size !== phases.length) fail('duplicate phase ids');

for (const p of phases) {
  const where = p.relFile;
  if (p.error || !p.fm) { fail(`${where}: front matter — ${p.error ?? 'unreadable'}`); continue; }
  const fm = p.fm;
  if (fm.phase !== p.file.slice(0, p.file.indexOf('-', 3))) fail(`${where}: phase id "${fm.phase}" does not match filename`);
  if (!PHASE_ID_RE.test(String(fm.phase))) fail(`${where}: invalid phase id`);
  if (typeof fm.title !== 'string' || fm.title.length === 0) fail(`${where}: title missing`);
  if (!STATUSES.includes(fm.status)) fail(`${where}: invalid status "${fm.status}"`);
  if (!trackRe.test(String(fm.track))) fail(`${where}: invalid track "${fm.track}"`);
  if (!cpRe.test(String(fm.checkpoint_id))) fail(`${where}: invalid checkpoint_id "${fm.checkpoint_id}"`);
  if (String(fm.checkpoint_id).charAt(3) !== String(fm.track)) fail(`${where}: checkpoint_id does not match track`);
  if (typeof fm.is_checkpoint !== 'boolean') fail(`${where}: is_checkpoint must be boolean`);
  if (!RISKS.includes(fm.risk)) fail(`${where}: invalid risk "${fm.risk}"`);
  if (!PERMISSION_CLASSES.includes(fm.permission_class)) fail(`${where}: invalid permission_class`);
  if (typeof fm.code_budget !== 'number') fail(`${where}: code_budget must be a number`);
  if (!fm.is_checkpoint && !(Number.isInteger(fm.owner_item) && fm.owner_item >= 1 && fm.owner_item <= OWNER_ITEMS)) fail(`${where}: owner_item must be 1…${OWNER_ITEMS} on a non-checkpoint phase`);
  // A late phase (`late: true`): an item the owner added after the checkpoints had closed. It takes an ordinal above
  // every checkpoint, depends on the last checkpoint or on a late phase before it, and belongs to a track whose
  // checkpoint is closed — which is then not asked to depend on it: a late phase carries its own walk.
  if (fm.late === true) {
    const lastCheckpoint = Math.max(...phases.filter((q) => q.fm?.is_checkpoint).map((q) => ordinalOf(q.id)[0]));
    const own = phases.find((q) => q.fm?.is_checkpoint && q.fm.checkpoint_id === fm.checkpoint_id);
    if (fm.is_checkpoint) fail(`${where}: a late phase is not a checkpoint`);
    if (ordinalOf(fm.phase)[0] <= lastCheckpoint) fail(`${where}: a late phase takes an ordinal above every checkpoint`);
    if (!(fm.depends_on ?? []).some((d) => byId.get(d)?.fm?.late === true || (byId.get(d)?.fm?.is_checkpoint === true && ordinalOf(d)[0] === lastCheckpoint))) fail(`${where}: a late phase depends on the last checkpoint or on a late phase before it`);
    if (own && !TERMINAL_OK.includes(own.fm.status)) fail(`${where}: late, but its track's checkpoint ${own.id} is not closed`);
  } else if (fm.late !== undefined && fm.late !== false) fail(`${where}: late must be true or false`);
  if (!fm.is_checkpoint && !fm.parent_phase && fm.late !== true && ordinalOf(fm.phase)[0] !== fm.owner_item) fail(`${where}: ordinal must match owner_item`);
  if (fm.parent_phase && !byId.has(fm.parent_phase)) fail(`${where}: unknown parent_phase`);
  if (!Array.isArray(fm.depends_on)) fail(`${where}: depends_on must be a list`);
  if (!Array.isArray(fm.unlocks)) fail(`${where}: unlocks must be a list`);
  if (!Array.isArray(fm.requirements) || fm.requirements.length === 0) fail(`${where}: requirements must be a non-empty list`);
  if (!Array.isArray(fm.read_scope) || fm.read_scope.length === 0) fail(`${where}: read_scope must be a non-empty list`);
  for (const scope of fm.read_scope ?? []) {
    if (typeof scope !== 'string' || scope === '') { fail(`${where}: read_scope entry must be a non-empty string`); continue; }
    if (scope.startsWith('spec: §')) continue;
    const after = afterRe.exec(scope);
    if (after) {
      if (!byId.has(after[1])) fail(`${where}: read_scope "after ${after[1]}" names an unknown phase`);
      else if (!transitiveDeps(fm.phase).has(after[1])) fail(`${where}: read_scope "after ${after[1]}" is not a dependency of ${fm.phase}`);
      continue;
    }
    if (!fs.existsSync(path.resolve(REPO_ROOT, scope))) fail(`${where}: missing read_scope path ${scope}`);
  }
  if ((fm.depends_on ?? []).length === 0 && roots.get(fm.track) !== fm.phase) fail(`${where}: only the lowest ordinal of track ${fm.track} (${roots.get(fm.track)}) may have no dependencies`);
  for (const d of fm.depends_on ?? []) {
    if (d === fm.phase) fail(`${where}: depends on itself`);
    else if (!byId.has(d)) fail(`${where}: unknown dependency ${d}`);
  }
  for (const r of fm.requirements ?? []) if (!reqIds.has(r)) fail(`${where}: unknown requirement ${r}`);
  for (const h of REQUIRED_HEADINGS) if (!p.body.includes(h)) fail(`${where}: missing section ${h}`);
  if (!p.body.includes('Canonical current execution state: `../AUTONOMOUS_STATE.md`')) fail(`${where}: missing resume pointer`);
  const h1 = p.body.match(/^# (.+)$/m)?.[1];
  if (h1 !== fm.title) fail(`${where}: H1 "${h1}" differs from title`);
  if (p.lines > 300) fail(`${where}: ${p.lines} lines exceeds the 300-line ceiling`);
  const record = path.join(EXECUTION_DIR, `${fm.phase}-EXECUTION.md`);
  if (TERMINAL_OK.includes(fm.status) && !fs.existsSync(record)) fail(`${where}: status ${fm.status} without ${path.relative(PROGRAMME_ROOT, record)}`);
  if (fm.status === 'READY' && !isEligible(p, byId)) fail(`${where}: READY but a dependency is not DONE`);
  if (ACTIVE_STATUSES.includes(fm.status) && !isEligible(p, byId)) fail(`${where}: ${fm.status} but a dependency is not DONE`);
}

// unlocks must be the exact inverse of depends_on
const inverse = new Map(phases.map((p) => [p.id, []]));
for (const p of phases) for (const d of p.fm?.depends_on ?? []) if (inverse.has(d)) inverse.get(d).push(p.id);
for (const p of phases) {
  if (!p.fm) continue;
  const expected = [...inverse.get(p.id)].sort().join(',');
  const actual = [...(p.fm.unlocks ?? [])].sort().join(',');
  if (expected !== actual) fail(`${p.relFile}: unlocks [${actual}] should be [${expected}] (run sync-progress.mjs --fix-unlocks)`);
}

const acyclic = checkAcyclic(phases.filter((p) => p.fm));
if (!acyclic.ok) fail(`dependency cycle among: ${acyclic.cycle.join(', ')}`);

// checkpoints: exactly one per checkpoint_id, depending on every other phase of its checkpoint
const byCp = new Map();
for (const p of phases) if (p.fm) { const k = p.fm.checkpoint_id; if (!byCp.has(k)) byCp.set(k, []); byCp.get(k).push(p); }
for (const [cp, list] of byCp) {
  const cps = list.filter((p) => p.fm.is_checkpoint);
  if (cps.length !== 1) { fail(`${cp}: expected exactly one checkpoint phase, found ${cps.length}`); continue; }
  const deps = new Set(cps[0].fm.depends_on);
  for (const p of list) if (!p.fm.is_checkpoint && p.fm.late !== true && !deps.has(p.id)) fail(`${cps[0].relFile}: checkpoint must depend on ${p.id}`);
}

// every owner item has at least one phase
const items = new Set(phases.map((p) => p.fm?.owner_item).filter((n) => Number.isInteger(n)));
for (let n = 1; n <= OWNER_ITEMS; n++) if (!items.has(n)) fail(`owner item ${n} has no phase`);

// at most one active phase
const active = phases.filter((p) => ACTIVE_STATUSES.includes(p.fm?.status));
if (active.length > 1) fail(`more than one active phase: ${active.map((p) => p.id).join(', ')}`);

// PROGRESS.json agreement
const progress = readProgress();
if (!progress) fail('PROGRESS.json missing (run sync-progress.mjs)');
else {
  if (progress.programId !== PROGRAM_ID) fail('PROGRESS.json: wrong programId');
  const pmap = new Map((progress.phases ?? []).map((x) => [x.id, x]));
  for (const p of phases) {
    const x = pmap.get(p.id);
    if (!x) { fail(`PROGRESS.json: missing ${p.id}`); continue; }
    if (x.status !== p.fm?.status) fail(`PROGRESS.json: ${p.id} status ${x.status} != ${p.fm?.status}`);
    if (x.file !== p.relFile) fail(`PROGRESS.json: ${p.id} file mismatch`);
    if ([...(x.dependsOn ?? [])].join(',') !== [...(p.fm?.depends_on ?? [])].join(',')) fail(`PROGRESS.json: ${p.id} dependsOn mismatch`);
  }
  for (const id of pmap.keys()) if (!byId.has(id)) fail(`PROGRESS.json: phantom phase ${id}`);
  if (progress.total !== phases.length) fail(`PROGRESS.json: total ${progress.total} != ${phases.length}`);
  const done = phases.filter((p) => p.fm?.status === 'DONE').length;
  if (progress.completed !== done) fail(`PROGRESS.json: completed ${progress.completed} != ${done}`);
  const expectedActive = active[0]?.id ?? null;
  if ((progress.activePhase ?? null) !== expectedActive) fail(`PROGRESS.json: activePhase ${progress.activePhase} != ${expectedActive}`);
  const expectedNext = expectedActive ? null : (pickNext(phases)?.id ?? null);
  if (progress.nextEligiblePhase !== expectedNext) fail('PROGRESS.json: nextEligiblePhase mismatch');
  if (progress.state === 'PLANNED' && phases.some((p) => !['NOT_STARTED', 'READY'].includes(p.fm?.status))) fail('PROGRESS.json: PLANNED after execution started');
  if (progress.state === 'COMPLETE' && phases.some((p) => !TERMINAL_OK.includes(p.fm?.status))) fail('PROGRESS.json: premature COMPLETE');
}

// AUTONOMOUS_STATE.md agreement (light)
if (!fs.existsSync(STATE_FILE)) fail('AUTONOMOUS_STATE.md missing');
else {
  const state = fs.readFileSync(STATE_FILE, 'utf8');
  const m = state.match(/^Active phase:\s*(.+)$/m);
  const declared = m ? m[1].trim() : null;
  const expected = active[0]?.id ?? 'none';
  if (declared !== expected) fail(`AUTONOMOUS_STATE.md: "Active phase: ${declared}" != ${expected}`);
}

// 300-line ceiling for every programme document and tool
for (const f of walkFiles(PROGRAMME_ROOT, (p) => /\.(md|mjs)$/.test(p))) {
  const n = countLines(f);
  if (n > 300) fail(`${path.relative(PROGRAMME_ROOT, f)}: ${n} lines exceeds the 300-line ceiling`);
}

for (const w of warnings) console.log(`WARN ${w}`);
for (const e of errors) console.log(`FAIL ${e}`);
console.log(`${errors.length === 0 ? 'PASS' : 'FAIL'}: ${phases.length} phases, ${reqIds.size} requirement ids, ${errors.length} error(s), ${warnings.length} warning(s)`);
process.exit(errors.length === 0 ? 0 : 1);
