// AS-001's walk (D-031): AvLoop's files read as AvLoop Studio's sequence by the built avloop-cli. Version-3 projects
// of each kind — plain, a feathered ellipse crop with a Cut shape, an inverse (the hole), main keyframes and a layer on
// V3 with its own keys — and a version-1 file: each one's tracks, clips, places and matte (the crop over everything,
// AS-002's reading RS-10); saved as version 4 with the
// "sequence" the CLI printed and every key of the file it read kept; read again — the same sequence, saved again —
// the same bytes; and rendered — a migrated file's frames byte for byte the version-3 file's.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-sequence-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const duration = Number(lab.probe(sample).format.duration);

function project(name, extra, version = 3) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version, source: { path: sample, stream: 0 }, range: { in: 0, out: 2 },
    transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' }, ...extra,
  }));
  return file;
}
function sequenceOf(file, save) {
  const r = lab.cli(['sequence', file, ...(save ? ['--save', save] : [])]);
  const line = r.out.split(/\r?\n/).find((l) => l.startsWith('{'));
  return r.status === 0 && line ? JSON.parse(line) : (w.note(`${path.basename(file)}: ${r.out.trim()}`), null);
}
function frame(file, k) {
  const raw = path.join(TMP, `${path.basename(file)}-${k}.rgba`);
  const r = lab.cli(['frame', file, '--at', String(k), '-o', `${raw}.png`, '--raw', raw]);
  return r.status === 0 ? fs.readFileSync(raw) : null;
}
const near = (a, b) => Math.abs(a - b) < 1e-6;
// Every value the file set is in the saved one (numbers to 1e-6): nothing renamed, nothing lost.
function within(a, b) {
  if (typeof a === 'number') return typeof b === 'number' && near(a, b);
  if (Array.isArray(a)) return Array.isArray(b) && a.length === b.length && a.every((v, i) => within(v, b[i]));
  if (a && typeof a === 'object') return !!b && typeof b === 'object' && Object.keys(a).every((k) => within(a[k], b[k]));
  return a === b;
}
// The file read as version 3, saved as 4, read and saved again: its sequence and the checks common to every kind.
function migrate(name, file) {
  const saved = path.join(TMP, `${name}-v4.avloop`), again = path.join(TMP, `${name}-v4b.avloop`);
  const s = sequenceOf(file, saved);
  const json = s && JSON.parse(fs.readFileSync(saved, 'utf8'));
  const input = JSON.parse(fs.readFileSync(file, 'utf8'));
  delete input.version;
  w.check(json?.version === 4 && within(s, json.sequence), `${name}: saved as version 4 with the sequence printed`,
    `version ${json?.version}`);
  w.check(json && within(input, json), `${name}: every key the file set kept, none renamed`, Object.keys(input).join(' '));
  const s2 = sequenceOf(saved, again);
  w.check(s2 && JSON.stringify(s2) === JSON.stringify(s), `${name}: read again — the same sequence`, 'equal');
  w.check(fs.existsSync(again) && fs.readFileSync(again).equals(fs.readFileSync(saved)),
    `${name}: saved again — the same bytes`, `${fs.existsSync(again) ? fs.statSync(again).size : 0} bytes`);
  return { s, saved };
}
const main = (s) => s.tracks[0].clips[0];

const plain = migrate('plain', project('plain', {}));
w.check(plain.s && plain.s.w === 640 && plain.s.h === 360 && plain.s.tracks.length === 1 && plain.s.fps === '30/1',
  'plain: a 640×360 sequence at 30/1, one track', plain.s && `${plain.s.w}×${plain.s.h} ${plain.s.fps} tracks ${plain.s.tracks.length}`);
const p = plain.s && main(plain.s);
w.check(p && p.w === 640 && p.h === 360 && Math.abs(p.duration - duration) < 0.05 && p.in === 0 && near(p.out, p.duration) &&
  p.x === 320 && p.y === 180 && p.masks.length === 0 && plain.s.in === 0 && plain.s.out === 2,
  'plain: V1 the whole video, centred, no mask; In/Out 0–2 s', p && `${p.w}×${p.h} ${p.duration}s at ${p.x},${p.y}`);

const crop = { kind: 'ellipse', x: 100, y: 50, w: 200, h: 100, feather: 10,
  shapes: [{ kind: 'rect', role: 'cut', x: 120, y: 60, w: 20, h: 20 }] };
const cropped = migrate('crop', project('crop', { crop }));
const c = cropped.s && main(cropped.s);
w.check(cropped.s && cropped.s.w === 200 && cropped.s.h === 100 && c.x === 220 && c.y === 130,
  'crop: the frame the crop\'s 200×100, the video placed from its corner', c && `${cropped.s.w}×${cropped.s.h} at ${c.x},${c.y}`);
const cm = cropped.s?.matte;
w.check(c && c.masks.length === 0 && cm.kind === 'ellipse' && cm.x === 0 && cm.y === 0 && cm.w === 200 &&
  cm.feather === 10 && !cm.invert && cm.shapes[0]?.role === 'cut' && cm.shapes[0]?.x === 20 && cm.shapes[0]?.y === 10,
  'crop: the matte — the ellipse at the frame\'s place, feathered 10, the Cut square with it',
  cm && `${cm.kind} ${cm.x},${cm.y} ${cm.w}×${cm.h} f${cm.feather} | ${cm.shapes.map((m) => `${m.role} ${m.x},${m.y}`)}`);

const hole = migrate('hole', project('hole', { crop: { ...crop, invert: true, shapes: [] } }));
const h = hole.s && main(hole.s);
const hm = hole.s?.matte;
w.check(hole.s && hole.s.w === 640 && hole.s.h === 360 && h.masks.length === 0 && hm.invert && hm.kind === 'ellipse' &&
  hm.x === 100 && hm.w === 200, 'hole: the whole video, the matte the ellipse inverted — the hole',
  hm && `${hole.s.w}×${hole.s.h} ${hm.kind} at ${hm.x},${hm.y} inverted ${hm.invert}`);

const layer = { name: 'counter', path: sample, w: 640, h: 360, duration, track: 3, start: 0.5, in: 1, out: 3, x: 160,
  y: 90, scale: 0.5, opacity: 80, visible: true, locked: false, keys: [[0, 160, 90, 0.5, 100, 0], [2, 480, 270, 0.5, 40, 1]] };
const keyed = migrate('layers', project('layers', {
  transform: { x: 320, y: 180, scale: 1, keyframes: [{ t: 1, x: 400, y: 200, scale: 1.5, interp: 'ease' }] }, layers: [layer] }));
const k = keyed.s;
const v3 = k && k.tracks[2]?.clips[0];
w.check(k && k.tracks.length === 3 && k.tracks.map((t) => t.name).join() === 'V1,V2,V3' && k.tracks[1].clips.length === 0,
  'layers: tracks V1, V2 (empty), V3', k && k.tracks.map((t) => `${t.name}:${t.clips.length}`).join(' '));
// The frame: the main video where it stands at In — held at its key (400, 200, ×1.5): 960×540 from (−80, −70).
const shifted = layer.keys.map(([t, x, y, ...rest]) => [t, x + 80, y + 70, ...rest]);
w.check(k && k.w === 960 && k.h === 540 && v3 && v3.start === 0.5 && v3.in === 1 && v3.out === 3 && v3.x === 240 &&
  v3.opacity === 80 && JSON.stringify(v3.keys) === JSON.stringify(shifted),
  'layers: a 960×540 frame from (−80, −70); the layer on V3 — its time, In/Out, place, opacity, keys from there',
  v3 && `start ${v3.start} ${v3.in}–${v3.out} at ${v3.x} ${v3.opacity}% keys ${JSON.stringify(v3.keys)}`);
w.check(k && JSON.stringify(main(k).keys) === JSON.stringify([[1, 480, 270, 1.5, 100, 1]]) && main(k).id !== v3.id,
  'layers: the main keyframe a clip key (opacity 100, Ease)', k && JSON.stringify(main(k).keys));

const old = migrate('version1', project('version1', { crop: { kind: 'rounded', x: 0, y: 0, w: 320, h: 180, radius: 20 } }, 1));
w.check(old.s && old.s.w === 320 && old.s.matte.kind === 'rounded' && old.s.matte.radius === 20,
  'version 1: read, the rounded crop the matte', old.s && `${old.s.w}×${old.s.h} ${old.s.matte.kind} r${old.s.matte.radius}`);

for (const [name, saved] of [['crop', cropped.saved], ['hole', hole.saved], ['layers', keyed.saved]]) {
  const a = frame(path.join(TMP, `${name}.avloop`), 30), b = frame(saved, 30);
  w.check(a && b && a.equals(b), `${name}: frame 30 of the version-4 file byte for byte the version-3 file's`,
    a && b ? `${a.length} bytes` : 'no frame');
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
