// AS-005's render walk (D-035): the project walk-effect-controls.json made — V2's 1080p counter at 25 %, keyed from
// (160, 90) at 0 s to (480, 270) and opacity 0 at 2 s, an ellipse mask over its middle made a hole (Cut), Invert on
// it alone — saved as controls.avloop, drawn by avloop-cli at sequence times and compared with the same project
// without V2: at 0 s V2's green bar inverted (magenta) at its own (400, 260); at 1 s half way and half seen (magenta
// half over V1); its hole showing V1 alone; at 2 s V2 gone (opacity 0): V1 alone. Both projects removed.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-controls-render-'));
const project = path.join(HERE, 'controls.avloop');
const alone = path.join(TMP, 'v1-alone.avloop');

function at(file, t) {
  const raw = path.join(TMP, `${path.basename(file)}-${t}.rgba`);
  const r = lab.cli(['sequence', file, '--at', String(t), '-o', `${raw}.png`, '--raw', raw]);
  const m = /frame (\d+)x(\d+)/.exec(r.out);
  return r.status === 0 && m ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : (w.note(r.out), null);
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
const near = (p, q, d) => p.every((v, i) => Math.abs(v - q[i]) <= d);

w.check(fs.existsSync(project), 'the project saved by the GUI walk', path.basename(project));
const json = JSON.parse(fs.readFileSync(project, 'utf8'));
const v2 = json.sequence?.tracks?.[1]?.clips?.[0];
w.check(v2 && v2.keys.length === 2 && v2.masks[0]?.role === 'cut' && v2.effects?.invert === true &&
  json.sequence.tracks[0].clips[0].effects?.invert !== true,
  'V2\'s clip: two keys, a Cut mask, Invert — the main clip without it',
  v2 ? `keys ${v2.keys.length}, mask ${v2.masks[0]?.role}, invert ${v2.effects?.invert}` : 'no V2 clip');
json.sequence.tracks[1].clips = [];
fs.writeFileSync(alone, JSON.stringify(json));

const a0 = at(project, 0), a1 = at(project, 1), a2 = at(project, 2), b1 = at(alone, 1), b2 = at(alone, 2);
// At 0 s V2 stands from (−80, −45): its own (400, 260) — the green bar, inverted — is the scene's (20, 20).
w.check(a0 && near(px(a0, 20, 20), [255, 0, 255, 255], 6), '0 s: V2 inverted at its own (400, 260): magenta',
  a0 ? px(a0, 20, 20).join(',') : 'no frame');
// At 1 s, 480 × 270 centred at (320, 180) — from (80, 45): its own (400, 260) is (180, 110); half seen over V1.
const half = b1 && px(b1, 180, 110).map((v, i) => (i === 3 ? 255 : Math.round(([255, 0, 255][i] + v) / 2)));
w.check(a1 && half && near(px(a1, 180, 110), half, 8), '1 s: half way, half seen — magenta half over V1',
  a1 && half ? `${px(a1, 180, 110)} (wanted ≈ ${half})` : 'no frame');
w.check(a1 && b1 && px(a1, 320, 180).join() === px(b1, 320, 180).join(), '1 s: through the hole, V1 alone',
  a1 && b1 ? `${px(a1, 320, 180)} = ${px(b1, 320, 180)}` : 'no frame');
w.check(a2 && b2 && a2.rgba.equals(b2.rgba), '2 s: opacity 0 — V1 alone, byte for byte', a2 ? `${a2.w}×${a2.h}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'controls.avloop-1.rgba.png'), path.join(HERE, 'render-1s.png'));
fs.rmSync(TMP, { recursive: true, force: true });
fs.rmSync(project, { force: true });
w.verdict();
