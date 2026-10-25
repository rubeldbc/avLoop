// Regenerates PROGRESS.json and control/00_PHASE_INDEX.md from the phase files.
// --fix-unlocks rewrites each phase file's `unlocks:` line as the exact inverse of depends_on.
import fs from 'node:fs';
import path from 'node:path';
import {
  PROGRAMME_ROOT, PROGRESS_FILE, INDEX_FILE, PROGRAM_ID, TRACKS, ACTIVE_STATUSES, TERMINAL_OK,
  loadPhases, readProgress, compareOrdinal, pickNext,
} from './lib.mjs';

const fixUnlocks = process.argv.includes('--fix-unlocks');
let phases = loadPhases();

if (fixUnlocks) {
  const inverse = new Map(phases.map((p) => [p.id, []]));
  for (const p of phases) for (const d of p.fm?.depends_on ?? []) if (inverse.has(d)) inverse.get(d).push(p.id);
  for (const p of phases) {
    const list = [...inverse.get(p.id)].sort(compareOrdinal);
    const text = fs.readFileSync(p.fullPath, 'utf8');
    const replaced = text.replace(/^unlocks:.*$/m, `unlocks: [${list.join(', ')}]`);
    if (replaced !== text) fs.writeFileSync(p.fullPath, replaced);
  }
  phases = loadPhases();
}

const existing = readProgress() ?? {};
const done = phases.filter((p) => p.fm?.status === 'DONE');
const active = phases.find((p) => ACTIVE_STATUSES.includes(p.fm?.status)) ?? null;
const next = pickNext(phases);
const everStarted = phases.some((p) => p.fm?.status !== 'NOT_STARTED' && p.fm?.status !== 'READY');
const allClosed = phases.every((p) => ['DONE', 'SPLIT', 'DEFERRED', 'BLOCKED_EXTERNAL'].includes(p.fm?.status));
let state = 'PLANNED';
if (allClosed && phases.every((p) => TERMINAL_OK.includes(p.fm?.status))) state = 'COMPLETE';
else if (allClosed) state = 'COMPLETE_WITH_BLOCKED';
else if (everStarted) state = 'ACTIVE';
if (existing.state === 'PAUSED') state = 'PAUSED';

const progress = {
  programId: existing.programId ?? PROGRAM_ID,
  programmeVersion: existing.programmeVersion ?? '1.0.0',
  generatedBy: 'control/phase-tools/sync-progress.mjs',
  generatedAt: new Date().toISOString(),
  state,
  activePhase: active?.id ?? null,
  nextEligiblePhase: active ? null : (next?.id ?? null),
  lastCompletedPhase: done.length ? done[done.length - 1].id : null,
  completed: done.length,
  total: phases.length,
  blocked: phases.filter((p) => p.fm?.status === 'BLOCKED_EXTERNAL').map((p) => p.id),
  deferred: phases.filter((p) => ['DEFERRED', 'FAILED_RETRYABLE'].includes(p.fm?.status)).map((p) => p.id),
  branch: existing.branch ?? 'main',
  publicationPolicy: existing.publicationPolicy ?? 'COMMIT_AND_PUSH_TO_ORIGIN_MAIN_AT_EVERY_PHASE_CLOSE',
  adrPrefix: existing.adrPrefix ?? 'AVADR',
  reservedAdrs: existing.reservedAdrs ?? [],
  readBudget: existing.readBudget ?? {},
  phases: phases.map((p) => ({
    id: p.id,
    title: p.fm?.title ?? '',
    status: p.fm?.status ?? 'UNKNOWN',
    track: p.fm?.track ?? '',
    checkpointId: p.fm?.checkpoint_id ?? '',
    isCheckpoint: p.fm?.is_checkpoint === true,
    risk: p.fm?.risk ?? '',
    dependsOn: p.fm?.depends_on ?? [],
    unlocks: p.fm?.unlocks ?? [],
    requirements: p.fm?.requirements ?? [],
    permissionClass: p.fm?.permission_class ?? '',
    codeBudget: p.fm?.code_budget ?? 0,
    ownerItem: p.fm?.owner_item ?? null,
    file: p.relFile,
    record: `execution/${p.id}-EXECUTION.md`,
  })),
};
fs.writeFileSync(PROGRESS_FILE, JSON.stringify(progress, null, 2) + '\n');

const classShort = {
  AUTO_ALLOWED: 'auto', VERIFY_ENVIRONMENT_FIRST: 'verify-env', EXTERNAL_CAPABILITY_REQUIRED: 'external',
  EXPLICIT_EXTERNAL_AUTHORIZATION_REQUIRED: 'authorize',
};
const rows = phases.map((p) => `| ${p.id} | ${p.fm.owner_item ?? '—'} | ${p.fm.track} | ${p.fm.checkpoint_id} | ${p.fm.status} | ${p.fm.risk} | ${classShort[p.fm.permission_class] ?? p.fm.permission_class} | ${p.fm.is_checkpoint ? '**' : ''}${p.fm.title}${p.fm.is_checkpoint ? '**' : ''} | ${(p.fm.depends_on ?? []).join(', ') || '—'} |`);
const index = [
  '<!-- generated-by: control/phase-tools/sync-progress.mjs — do not edit by hand -->',
  '# 00 — Phase Index',
  '',
  'Canonical current execution state: `../AUTONOMOUS_STATE.md`',
  '',
  `Phases: **${phases.length}** · done: **${done.length}** · state: **${state}** · generated: ${progress.generatedAt}`,
  '',
  'Tracks: ' + Object.entries(TRACKS).map(([k, v]) => `${k} ${v}`).join(' · '),
  '',
  '"Item" is the owner\'s numbered item the phase implements (`../README.md` §1).',
  '',
  '| Phase | Item | Track | CP | Status | Risk | Class | Title | Depends on |',
  '| --- | --- | --- | --- | --- | --- | --- | --- | --- |',
  ...rows,
  '',
].join('\n');
fs.writeFileSync(INDEX_FILE, index);

console.log(`sync-progress: ${phases.length} phases, ${done.length} done, state ${state}, active ${active?.id ?? 'none'}, next ${progress.nextEligiblePhase ?? 'none'}${fixUnlocks ? ' (unlocks fixed)' : ''}`);
console.log(`wrote ${path.relative(PROGRAMME_ROOT, PROGRESS_FILE)} and ${path.relative(PROGRAMME_ROOT, INDEX_FILE)}`);
