// AS-006's render walk (D-036): the project walk-masks.json made — V2's 1080p counter at 25 %, an ellipse drawn on the
// monitor over its middle as a Cut mask, then the clip dragged 80 px right — saved as masks.avloop, drawn by
// avloop-cli at 0 s and compared with the same project without V2: the mask is in V2's own px (centred on its middle,
// 480 × 320 of its 1920 × 1080), the crop has no shapes; the hole moved with the clip — at its new centre (400, 180)
// V1 alone, on V2 beside the hole (400, 60) not V1. Both projects removed.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-masks-render-'));
const project = path.join(HERE, 'masks.avloop');
const alone = path.join(TMP, 'v1-alone.avloop');

function at(file, t) {
  const raw = path.join(TMP, `${path.basename(file)}-${t}.rgba`);
  const r = lab.cli(['sequence', file, '--at', String(t), '-o', `${raw}.png`, '--raw', raw]);
  const m = /frame (\d+)x(\d+)/.exec(r.out);
  return r.status === 0 && m ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : (w.note(r.out), null);
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];

w.check(fs.existsSync(project), 'the project saved by the GUI walk', path.basename(project));
const json = JSON.parse(fs.readFileSync(project, 'utf8'));
const v2 = json.sequence?.tracks?.[1]?.clips?.[0];
const m = v2?.masks?.[0];
w.check(m && m.kind === 'ellipse' && m.role === 'cut' && Math.abs(m.x + m.w / 2 - 960) <= 4 &&
  Math.abs(m.y + m.h / 2 - 540) <= 4 && Math.abs(m.w - 480) <= 4 && Math.abs(m.h - 320) <= 4,
  'V2\'s mask in its own px: an ellipse Cut, centred on its middle, 480 × 320',
  m ? `${m.kind} ${m.role} ${m.x},${m.y} ${m.w}×${m.h}` : 'no mask');
w.check(v2 && Math.abs(v2.x - 400) <= 1 && (json.sequence.matte?.shapes ?? []).length === 0, 'the clip moved to x 400 (a drag: within 1 px); the crop: no shapes',
  v2 ? `x ${v2.x}, crop shapes ${(json.sequence.matte?.shapes ?? []).length}` : 'no V2');
json.sequence.tracks[1].clips = [];
fs.writeFileSync(alone, JSON.stringify(json));
const a = at(project, 0), b = at(alone, 0);
w.check(a && b && px(a, 400, 180).join() === px(b, 400, 180).join(), 'the hole moved with it: (400, 180) V1 alone',
  a && b ? `${px(a, 400, 180)} = ${px(b, 400, 180)}` : 'no frame');
w.check(a && b && px(a, 400, 60).join() !== px(b, 400, 60).join(), 'beside the hole (400, 60): V2, not V1',
  a && b ? `${px(a, 400, 60)} ≠ ${px(b, 400, 60)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'masks.avloop-0.rgba.png'), path.join(HERE, 'render-0s.png'));
fs.rmSync(TMP, { recursive: true, force: true });
fs.rmSync(project, { force: true });
w.verdict();
