// AV-027's render walk (D-024): soft shapes in the export's own frames. The counter sample, no crop: a Select
// rectangle (60, 40, 520, 280) with a Cut ellipse (220, 100, 200, 160) in it — Strength 40: the ellipse's centre keeps
// 60 % (alpha 153), outside it 255; Feather 30 too: from the ellipse's edge inward the alpha falls from 255 to 153 over
// ~30 px; a feathered polygon by the distance transform; the defaults byte for byte AV-024's; GPU = CPU.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-soft-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');

function project(name, shapes) {
  const file = path.join(TMP, `${name}.avloop`);
  fs.writeFileSync(file, JSON.stringify({
    format: 'avloop', version: 3,
    source: { path: sample.split(path.sep).join('/'), stream: 0 },
    range: { in: 0, out: 2 }, transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
    crop: { kind: 'rect', x: 0, y: 0, w: 0, h: 0, ratio: 'free', shapes },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  }));
  return file;
}
function frame(file, cpu) {
  const raw = path.join(TMP, `${path.basename(file)}${cpu ? '-cpu' : ''}.rgba`);
  const r = lab.cli(['frame', file, '--at', '0', '-o', `${raw}.png`, '--raw', raw, ...(cpu ? ['--cpu'] : [])]);
  const m = /(\d+)x(\d+)/.exec(r.out);
  return r.status === 0 ? { w: Number(m[1]), h: Number(m[2]), rgba: fs.readFileSync(raw) } : null;
}
const alpha = (f, x, y) => f.rgba[(y * f.w + x) * 4 + 3];
const maxDiff = (a, b) => a.rgba.reduce((m, v, i) => Math.max(m, Math.abs(v - b.rgba[i])), 0);
const rect = { kind: 'rect', role: 'add', x: 60, y: 40, w: 520, h: 280 };
const ellipse = (extra) => ({ kind: 'ellipse', role: 'cut', x: 220, y: 100, w: 200, h: 160, ...extra });

const weak = frame(project('weak', [rect, ellipse({ strength: 40 })]));
w.check(Math.abs(alpha(weak, 320, 180) - 153) <= 1, 'Strength 40: the cut\'s centre keeps 60 %', `alpha ${alpha(weak, 320, 180)}`);
w.check(alpha(weak, 100, 180) === 255 && alpha(weak, 20, 180) === 0, 'outside the cut 255; outside the rectangle 0',
  `${alpha(weak, 100, 180)}, ${alpha(weak, 20, 180)}`);

const soft = frame(project('soft', [rect, ellipse({ strength: 40, feather: 30 })]));
const ramp = [219, 222, 230, 240, 250, 260].map((x) => alpha(soft, x, 180));
w.check(ramp[0] === 255 && ramp.every((a, i) => i === 0 || a <= ramp[i - 1]) && Math.abs(ramp[5] - 153) <= 1,
  'Feather 30: from the edge (x 220) inward 255 falls to 153 over ~30 px', ramp.join(' → '));
w.check(Math.abs(alpha(soft, 320, 180) - 153) <= 1, 'the centre as without feather', `alpha ${alpha(soft, 320, 180)}`);

const tri = { kind: 'polygon', role: 'add', points: [[100, 60], [540, 60], [100, 300]], feather: 20 };
const t = frame(project('tri', [tri]));
const down = [60, 65, 70, 75, 80, 85].map((y) => alpha(t, 200, y));
w.check(down[0] < 40 && down.every((a, i) => i === 0 || a >= down[i - 1]) && down[5] === 255,
  'a feathered polygon (the distance transform): rising inward from its top edge, full by ~20 px', down.join(' → '));

const plain = frame(project('plain', [rect, ellipse({})]));
const explicit = frame(project('explicit', [rect, ellipse({ feather: 0, strength: 100 })]));
w.check(plain && explicit && plain.rgba.equals(explicit.rgba) && alpha(plain, 320, 180) === 0,
  'the defaults: AV-024\'s frame byte for byte (the cut\'s centre 0)', `alpha ${alpha(plain, 320, 180)}`);

const cpu = frame(project('soft', [rect, ellipse({ strength: 40, feather: 30 })]), true);
w.check(cpu && maxDiff(soft, cpu) <= 2, 'the GPU and the CPU: the same picture', cpu ? `max diff ${maxDiff(soft, cpu)}` : 'no frame');
fs.copyFileSync(path.join(TMP, 'soft.avloop.rgba.png'), path.join(HERE, 'render-soft.png'));
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
