// AS-002's render walk (D-032): the composite by the built avloop-cli on the counter sample (640×360, 30 fps, its
// frame number in 16 stripes along its foot). AvLoop's one video under an ellipse crop with keyframes: the composite
// within 1/255 of AvLoop's own path; a project with a layer: the composite, GPU = CPU byte for byte; a hand-made
// version-4 sequence — the Razor's two halves on V1 showing their own media times, a Cut ellipse in V2 showing V1
// through it, an inverted matte's hole clear, V2 keyed to half opacity; 1,200 frames of a layers project: memory flat.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-composite-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');

const base = { format: 'avloop', source: { path: sample, stream: 0 }, range: { in: 0, out: 2 },
  transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
  output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' } };
function file(name, extra, version = 3) {
  const f = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(f, JSON.stringify({ ...base, version, ...extra }));
  return f;
}
const read = (raw, m) => (fs.existsSync(raw) ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : null);
function frame(f, k, cpu) {
  const raw = path.join(TMP, `${path.basename(f)}-${k}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', f, '--at', String(k), '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  const m = /(\d+)x(\d+)/.exec(r.out);
  return r.status === 0 && m ? { ...read(raw, m), out: r.out } : (w.note(r.out.trim()), null);
}
function seqAt(f, t, keep) {
  const raw = path.join(TMP, `${path.basename(f)}-at${t}.rgba`);
  const r = lab.cli(['sequence', f, '--at', String(t), '-o', `${raw}.png`, '--raw', raw]);
  const m = /frame (\d+)x(\d+)/.exec(r.out);
  if (keep) fs.copyFileSync(`${raw}.png`, path.join(HERE, keep));
  return r.status === 0 && m ? read(raw, m) : (w.note(r.out.trim()), null);
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);
function counter(f, foot = 16) { // the counter's 16 stripes at its foot (tests/Samples.h's reading)
  let n = 0;
  for (let k = 0; k < 16; k++) {
    const p = px(f, Math.floor(k * f.w / 16 + f.w / 32), f.h - foot);
    if ((p[0] + p[1] + p[2]) / 3 > 128) n |= 1 << k;
  }
  return n;
}

// 1. AvLoop's one video: its own path and the composite.
const one = file('one', { crop: { kind: 'ellipse', x: 160, y: 40, w: 280, h: 280, feather: 16 },
  transform: { x: 320, y: 180, scale: 1, keyframes: [{ t: 0, x: 320, y: 180, scale: 1.2, interp: 'linear' },
    { t: 2, x: 300, y: 200, scale: 1.4, interp: 'ease' }] } });
const own = frame(one, 30, true), comp = seqAt(one, 1);
w.check(own && comp && own.w === comp.w && maxDiff(own, comp) <= 1,
  'one video, an ellipse crop, keyframes: the composite within 1/255 of AvLoop\'s own path at 1 s',
  own && comp ? `${comp.w}×${comp.h}, max diff ${maxDiff(own, comp)}` : 'no frame');

// 2. A project with a layer: the composite on either setting, the same bytes.
const layer = { name: 'counter', path: sample, w: 640, h: 360, duration: 10, track: 2, start: 0, in: 3, out: 10, x: 160,
  y: 90, scale: 0.5, opacity: 100, visible: true, locked: false };
const layered = file('layered', { layers: [layer] });
const g = frame(layered, 30), c = frame(layered, 30, true);
w.check(g && c && g.rgba.equals(c.rgba) && /CPU/.test(g.out), 'a layer: the composite, GPU setting = CPU byte for byte',
  g ? g.out.trim().split('\n').pop() : 'no frame');
const q = g && { ...g, w: 320, h: 180, rgba: Buffer.alloc(320 * 180 * 4) };
if (q) for (let y = 0; y < 180; y++) g.rgba.copy(q.rgba, y * 320 * 4, y * 640 * 4, y * 640 * 4 + 320 * 4);
w.check(g && counter(g) === 30 && counter(q, 8) === 120, 'its counters: the main video\'s frame 30, the layer\'s 120 (3 s on)',
  g ? `${counter(g)} | ${counter(q, 8)}` : 'no frame');

// 3. A sequence made by hand (version 4): the Razor's halves, a hole in V2, the matte, opacity.
const clip = (id, extra) => ({ id, name: 'counter', path: sample, w: 640, h: 360, duration: 10, start: 0, in: 0, out: 10,
  x: 320, y: 180, scale: 1, flipH: false, flipV: false, keys: [], opacity: 100, effects: {}, masks: [], enabled: true,
  ...extra });
const seq = (v1, v2, matte = {}) => file(`seq${Math.random().toString(36).slice(2, 7)}`, { sequence: { w: 640, h: 360,
  fps: '30/1', in: 0, out: 10, nextId: 9, matte, tracks: [{ name: 'V1', visible: true, locked: false, clips: v1 },
    { name: 'V2', visible: true, locked: false, clips: v2 }] } }, 4);
const halves = seq([clip(1, { out: 2 }), clip(2, { start: 2, in: 3 })], []);
const h1 = seqAt(halves, 1.5), h2 = seqAt(halves, 2.5, 'razor-2.5s.png');
w.check(h1 && h2 && counter(h1) === 45 && counter(h2) === 105,
  'the Razor\'s halves: at 1.5 s the media\'s frame 45, at 2.5 s (the second half from 3 s) frame 105',
  h1 && h2 ? `${counter(h1)} | ${counter(h2)}` : 'no frame');
const cut = { kind: 'ellipse', role: 'cut', name: 'Hole', visible: true, locked: false, feather: 0, strength: 100,
  x: 100, y: 130, w: 200, h: 100, radius: 0 };
const flipped = clip(1, { flipH: true }); // V1 mirrored: other colours than V2's under the hole and around it
const below = seqAt(seq([flipped], []), 0), above = seqAt(seq([], [clip(2, { in: 5 })]), 0);
const holed = seqAt(seq([flipped], [clip(2, { in: 5, masks: [cut] })]), 0, 'hole-in-v2.png');
const same = (a, b, x, y) => px(a, x, y).join() === px(b, x, y).join();
w.check(holed && same(holed, below, 200, 180) && !same(holed, above, 200, 180) && same(holed, above, 40, 40) &&
  !same(holed, below, 40, 40) && counter(holed) === 150,
  'a Cut ellipse in V2: V1 through the hole, V2 around it (its foot: frame 150)',
  holed ? `centre ${px(holed, 200, 180)} = V1's (V2 there: ${px(above, 200, 180)}), corner ${px(holed, 40, 40)} = V2's, foot ${counter(holed)}` : 'no frame');
const matte = seqAt(seq([clip(1)], [], { kind: 'ellipse', x: 220, y: 130, w: 200, h: 100, feather: 8, invert: true,
  shapes: [] }), 0, 'matte-hole.png');
w.check(matte && px(matte, 320, 180)[3] === 0 && px(matte, 40, 40)[3] === 255,
  'the matte inverted: its ellipse clear through every track, the rest opaque',
  matte ? `alpha ${px(matte, 320, 180)[3]} | ${px(matte, 40, 40)[3]}` : 'no frame');
const fade = seqAt(seq([], [clip(2, { keys: [[0, 320, 180, 1, 0, 0], [2, 320, 180, 1, 100, 0]] })]), 1);
w.check(fade && Math.abs(px(fade, 40, 40)[3] - 128) <= 1, 'V2 keyed 0 → 100 % over 2 s: half at 1 s',
  fade ? `alpha ${px(fade, 40, 40)[3]}` : 'no frame');

// 4. Memory: 1,200 frames of a project with three layers, by the second.
const three = file('three', { range: { in: 0, out: 4 },
  layers: [2, 3, 4].map((track) => ({ ...layer, track, x: 100 * track, scale: 0.3 * (track - 1) })) });
const csv = path.join(TMP, 'mem.csv');
const run = lab.cli(['frames', three, '--count', '1200', '--mem-log', csv]);
const rows = fs.existsSync(csv) ? fs.readFileSync(csv, 'utf8').trim().split('\n').slice(1).map((l) => l.split(',').map(Number)) : [];
const mb = rows.map((r) => r[1]);
const grew = mb.length >= 3 ? Math.max(...mb.slice(1)) - mb[1] : NaN;
w.check(run.status === 0 && mb.length >= 3 && grew <= 8, '1,200 frames with three layers: private bytes flat after the first second',
  `${run.out.trim().split('\n').pop()} · ${mb.join(' → ')} MB (+${grew.toFixed(1)})`);

fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
