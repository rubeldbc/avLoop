// Shared helpers for the AVLOOP phase tools: the owner's example programme's helpers (CABTV-AIRTICKER2) ported with
// this programme's id, its prefix (`AV-`), its two tracks and its seventeen items.
// Node built-ins only. The root rule (`trackRoots`) and the chain rule's pick (`pickNext`): a FAILED_RETRYABLE phase
// first, then a checkpoint whose track is done, then the lowest ordinal.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

export const TOOLS_DIR = path.dirname(fileURLToPath(import.meta.url));
export const PROGRAMME_ROOT = path.resolve(TOOLS_DIR, '..', '..');
export const REPO_ROOT = path.resolve(PROGRAMME_ROOT, '..', '..');
export const PHASES_DIR = path.join(PROGRAMME_ROOT, 'phases');
export const EXECUTION_DIR = path.join(PROGRAMME_ROOT, 'execution');
export const REQUIREMENTS_DIR = path.join(PROGRAMME_ROOT, 'requirements');
export const PROGRESS_FILE = path.join(PROGRAMME_ROOT, 'PROGRESS.json');
export const STATE_FILE = path.join(PROGRAMME_ROOT, 'AUTONOMOUS_STATE.md');
export const INDEX_FILE = path.join(PROGRAMME_ROOT, 'control', '00_PHASE_INDEX.md');

export const PROGRAM_ID = 'AVLOOP';
export const PHASE_PREFIX = 'AV';
export const TRACKS = {
  A: 'The loop maker — the ground, the media, the canvas and the Selection, the crop tools, the timeline, the loop engine, the renderer, the preview window, the export, the project and the settings',
  B: 'The extras — the keyframes, the loop finder, the batch and the other formats, the installer, the name and the About box, the wheel and the keyframe copy, the clip effects',
};

/**
 * The items of `README.md` §2: 17. A non-checkpoint phase's ordinal is its item. A checkpoint takes the free ordinal
 * after the last item (CP-A1 = AV-015, CP-B1 = AV-016). An item the owner adds after a checkpoint has closed is a late
 * phase — `late: true`, the next free ordinal above it (`control/01_PHASE_DEPENDENCY_GRAPH.md`): item 15, AV-017;
 * item 16, AV-018; item 17, AV-019.
 */
export const OWNER_ITEMS = 17;

export const STATUSES = [
  'NOT_STARTED', 'READY', 'IN_PROGRESS', 'TESTING', 'RECOVERY', 'BLOCKED_EXTERNAL',
  'DONE', 'FAILED_RETRYABLE', 'DEFERRED', 'SPLIT',
];
export const ACTIVE_STATUSES = ['IN_PROGRESS', 'TESTING', 'RECOVERY'];
export const TERMINAL_OK = ['DONE', 'SPLIT'];
export const RISKS = ['Low', 'Medium', 'High'];
export const PERMISSION_CLASSES = [
  'AUTO_ALLOWED', 'VERIFY_ENVIRONMENT_FIRST', 'EXTERNAL_CAPABILITY_REQUIRED',
  'EXPLICIT_EXTERNAL_AUTHORIZATION_REQUIRED',
];
export const REQUIRED_HEADINGS = [
  '**Goal.**', '**Ground truth**', '**Do**', '**Do not**', '**Done when**',
  '**Decisions this phase must record**', '**Validation**', '**Seam if this splits.**',
];
export const PHASE_ID_RE = /^AV-\d{3}[a-z]?$/;
const PHASE_FILE_RE = /^AV-\d{3}[a-z]?-.*\.md$/;

function parseScalar(raw) {
  const v = raw.trim();
  if (v === '') return '';
  if (v === 'true') return true;
  if (v === 'false') return false;
  if (v === 'null' || v === '~') return null;
  if (/^-?\d+$/.test(v)) return Number(v);
  if ((v.startsWith('"') && v.endsWith('"')) || (v.startsWith("'") && v.endsWith("'"))) return v.slice(1, -1);
  return v;
}

function parseInlineList(raw) {
  const inner = raw.trim().slice(1, -1).trim();
  if (inner === '') return [];
  return inner.split(',').map((s) => parseScalar(s));
}

/** Minimal YAML-subset front matter parser: scalars, inline lists, block lists of strings. */
export function parseFrontMatter(text) {
  const norm = text.replace(/\r\n/g, '\n');
  if (!norm.startsWith('---\n')) return { data: null, body: norm, error: 'missing front matter' };
  const end = norm.indexOf('\n---', 4);
  if (end < 0) return { data: null, body: norm, error: 'unterminated front matter' };
  const block = norm.slice(4, end);
  const body = norm.slice(end + 4).replace(/^\n+/, '');
  const data = {};
  let currentKey = null;
  for (const line of block.split('\n')) {
    if (line.trim() === '') continue;
    const listItem = line.match(/^\s+-\s?(.*)$/);
    if (listItem && currentKey) {
      if (!Array.isArray(data[currentKey])) data[currentKey] = [];
      data[currentKey].push(listItem[1].trim());
      continue;
    }
    const kv = line.match(/^([A-Za-z_][A-Za-z0-9_]*):(.*)$/);
    if (!kv) return { data: null, body, error: `cannot parse line: ${line}` };
    const key = kv[1];
    const rest = kv[2];
    currentKey = key;
    const trimmed = rest.trim();
    if (trimmed === '') data[key] = [];
    else if (trimmed.startsWith('[')) data[key] = parseInlineList(trimmed);
    else data[key] = parseScalar(trimmed);
  }
  return { data, body, error: null };
}

/** `AV-004a` → [4, 'a']; anything else sorts last. */
export function ordinalOf(id) {
  const m = /^AV-(\d{3})([a-z]?)$/.exec(String(id));
  return m ? [Number(m[1]), m[2]] : [Number.POSITIVE_INFINITY, ''];
}

export function compareOrdinal(a, b) {
  const [na, sa] = ordinalOf(a);
  const [nb, sb] = ordinalOf(b);
  return na !== nb ? na - nb : sa.localeCompare(sb);
}

/**
 * The phases allowed to have no dependency: the lowest ordinal of each track (`control/01` §Roots).
 * Returns a Map track → phase id.
 */
export function trackRoots(phases) {
  const roots = new Map();
  for (const p of phases) {
    const track = p.fm?.track;
    if (!track) continue;
    const current = roots.get(track);
    if (!current || compareOrdinal(p.id, current) < 0) roots.set(track, p.id);
  }
  return roots;
}

export function listPhaseFiles() {
  if (!fs.existsSync(PHASES_DIR)) return [];
  return fs.readdirSync(PHASES_DIR).filter((f) => PHASE_FILE_RE.test(f)).sort();
}

export function loadPhases() {
  return listPhaseFiles().map((file) => {
    const full = path.join(PHASES_DIR, file);
    const text = fs.readFileSync(full, 'utf8');
    const { data, body, error } = parseFrontMatter(text);
    return {
      id: data?.phase ?? file.slice(0, file.indexOf('-', 3)),
      file,
      relFile: `phases/${file}`,
      fullPath: full,
      fm: data,
      body,
      error,
      lines: text.split(/\r?\n/).length,
    };
  });
}

export function loadRequirementIds() {
  const ids = new Set();
  if (!fs.existsSync(REQUIREMENTS_DIR)) return ids;
  for (const f of fs.readdirSync(REQUIREMENTS_DIR)) {
    if (!f.endsWith('.md')) continue;
    const text = fs.readFileSync(path.join(REQUIREMENTS_DIR, f), 'utf8');
    for (const m of text.matchAll(/\|\s*(REQ-[A-Z0-9]+-\d{3})\s*\|/g)) ids.add(m[1]);
  }
  return ids;
}

export function walkFiles(dir, predicate, out = []) {
  if (!fs.existsSync(dir)) return out;
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walkFiles(p, predicate, out);
    else if (predicate(p)) out.push(p);
  }
  return out;
}

export function countLines(file) {
  const text = fs.readFileSync(file, 'utf8');
  if (text === '') return 0;
  return text.split(/\r?\n/).length - (text.endsWith('\n') ? 1 : 0);
}

/** Kahn topological check; returns { ok, cycle } */
export function checkAcyclic(phases) {
  const ids = new Set(phases.map((p) => p.id));
  const indeg = new Map();
  const adj = new Map();
  for (const p of phases) { indeg.set(p.id, 0); adj.set(p.id, []); }
  for (const p of phases) {
    for (const d of p.fm?.depends_on ?? []) {
      if (!ids.has(d)) continue;
      adj.get(d).push(p.id);
      indeg.set(p.id, indeg.get(p.id) + 1);
    }
  }
  const queue = [...indeg.entries()].filter(([, n]) => n === 0).map(([id]) => id);
  let seen = 0;
  while (queue.length) {
    const id = queue.shift();
    seen++;
    for (const n of adj.get(id)) {
      indeg.set(n, indeg.get(n) - 1);
      if (indeg.get(n) === 0) queue.push(n);
    }
  }
  if (seen === phases.length) return { ok: true, cycle: [] };
  return { ok: false, cycle: [...indeg.entries()].filter(([, n]) => n > 0).map(([id]) => id) };
}

export function readProgress() {
  if (!fs.existsSync(PROGRESS_FILE)) return null;
  return JSON.parse(fs.readFileSync(PROGRESS_FILE, 'utf8'));
}

export function isEligible(phase, byId) {
  return (phase.fm?.depends_on ?? []).every((d) => TERMINAL_OK.includes(byId.get(d)?.fm?.status));
}

/**
 * The chain rule's next phase: among NOT_STARTED/READY phases whose dependencies are all done, a phase of a
 * failed-retryable state first, then a checkpoint (its dependencies being done means its track is), then the lowest
 * ordinal. Returns the phase or null.
 */
export function pickNext(phases) {
  const byId = new Map(phases.map((p) => [p.id, p]));
  const retry = phases.filter((p) => p.fm?.status === 'FAILED_RETRYABLE' && isEligible(p, byId));
  const open = phases.filter((p) => ['NOT_STARTED', 'READY'].includes(p.fm?.status) && isEligible(p, byId));
  const rank = (p) => (p.fm?.status === 'FAILED_RETRYABLE' ? 0 : p.fm?.is_checkpoint === true ? 1 : 2);
  const candidates = [...retry, ...open].sort((a, b) => rank(a) - rank(b) || compareOrdinal(a.id, b.id));
  return candidates[0] ?? null;
}
