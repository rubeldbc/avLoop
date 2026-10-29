// AS-008's render walk (D-038): studio.avloop as walk-project.json saved it — version 5, the sequence and the bin —
// drawn by avloop-cli: at 13 s the cut clip from its media's 3 s (frame 90), at 17 s the last clip from its 7 s
// (frame 210) with the hole clear at its middle; avloop-v3.avloop as it was written (version 3, never saved over).
// Both files removed after.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-project-render-'));
const studio = path.join(HERE, 'studio.avloop');
const v3 = path.join(HERE, 'avloop-v3.avloop');

function at(t) {
  const raw = path.join(TMP, `at${t}.rgba`);
  const r = lab.cli(['sequence', studio, '--at', String(t), '-o', `${raw}.png`, '--raw', raw]);
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

const json = fs.existsSync(studio) ? JSON.parse(fs.readFileSync(studio, 'utf8')) : null;
w.check(json?.version === 5 && json.sequence?.tracks?.[0]?.clips?.length === 4 && json.media?.length === 2,
  'studio.avloop: version 5, four clips on V1, two media in the bin',
  json ? `version ${json.version}, V1 ${json.sequence?.tracks?.[0]?.clips?.length}, media ${json.media?.length}` : 'missing');
const a = at(13), b = at(17);
w.check(a && counter(a) === 90, '13 s: the cut clip from its media\'s 3 s — frame 90', a ? `frame ${counter(a)}` : 'no frame');
w.check(b && counter(b) === 210 && px(b, 320, 180)[3] === 0, '17 s: the last clip from its 7 s — frame 210, the hole clear',
  b ? `frame ${counter(b)}, alpha ${px(b, 320, 180)[3]}` : 'no frame');
const old = fs.existsSync(v3) ? JSON.parse(fs.readFileSync(v3, 'utf8')) : null;
w.check(old?.version === 3 && !old.sequence, 'avloop-v3.avloop untouched: version 3, no sequence (never saved over)',
  old ? `version ${old.version}` : 'missing');
fs.rmSync(TMP, { recursive: true, force: true });
fs.rmSync(studio, { force: true });
fs.rmSync(v3, { force: true });
w.verdict();
