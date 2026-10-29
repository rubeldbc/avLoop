// AS-004's render walk (D-034): the project walk-timeline-edit.json cut, deleted, rippled, moved and trimmed — saved as
// edited.avloop — drawn by the built avloop-cli at sequence times (the composite), its counters read: the first clip
// cut at 1 s shows the media's frames 15 and 45 either side; the second half of the old last clip (rippled to 2 s, from
// its media's 2 s) frame 90 at 3 s; nothing at 5.5 s (a gap, transparent); V2's clip from its media's 6 s at 7 s:
// frame 210. The saved project is removed after.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-timeline-render-'));
const project = path.join(HERE, 'edited.avloop');

function at(t) {
  const raw = path.join(TMP, `at${t}.rgba`);
  const r = lab.cli(['sequence', project, '--at', String(t), '-o', `${raw}.png`, '--raw', raw]);
  const m = /frame (\d+)x(\d+)/.exec(r.out);
  return r.status === 0 && m ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : (w.note(r.out), null);
}
const px = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 4)];
function counter(f) {
  let n = 0;
  for (let k = 0; k < 16; k++) {
    const p = px(f, Math.floor(k * f.w / 16 + f.w / 32), f.h - 16);
    if ((p[0] + p[1] + p[2]) / 3 > 128) n |= 1 << k;
  }
  return n;
}

w.check(fs.existsSync(project), 'the edited project saved by the GUI walk', path.basename(project));
const json = JSON.parse(fs.readFileSync(project, 'utf8'));
w.check(json.version === 5 && json.sequence?.tracks?.[0]?.clips?.length === 3, 'version 5, three clips on V1',
  `version ${json.version}, V1 ${json.sequence?.tracks?.[0]?.clips?.length}`);
for (const [t, want, what] of [[0.5, 15, 'the first clip before the cut'], [1.5, 45, 'after the cut at 1 s'],
  [3, 90, 'the rippled clip, from its media\'s 2 s'], [7, 210, 'V2\'s clip, from its media\'s 6 s']]) {
  const f = at(t);
  w.check(f && f.w === 640 && counter(f) === want, `${t} s: ${what} — frame ${want}`, f ? `${f.w}×${f.h}, frame ${counter(f)}` : 'no frame');
}
const gap = at(5.5);
w.check(gap && px(gap, 320, 180)[3] === 0 && px(gap, 40, 300)[3] === 0, '5.5 s: the gap — transparent',
  gap ? `alpha ${px(gap, 320, 180)[3]}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'at3.rgba.png'), path.join(HERE, 'render-3s.png'));
fs.rmSync(TMP, { recursive: true, force: true });
fs.rmSync(project, { force: true });
w.verdict();
