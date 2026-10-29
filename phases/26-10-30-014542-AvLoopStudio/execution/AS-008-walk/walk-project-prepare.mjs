// AS-008's first walk: an AvLoop project of version 3 written beside the walks (avloop-v3.avloop) — the counter under
// an ellipse crop with a feather, In 0 → Out 4, a layer of the counter at half size on V2 from 1 s — for
// walk-project.json to open as a sequence; walk-project-render.mjs removes it.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const file = path.join(HERE, 'avloop-v3.avloop');
fs.writeFileSync(file, JSON.stringify({
  format: 'avloop', version: 3, source: { path: sample, stream: 0 }, range: { in: 0, out: 4 },
  transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'simple' },
  crop: { kind: 'ellipse', x: 120, y: 40, w: 400, h: 280, feather: 8 },
  output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1' },
  layers: [{ name: 'counter', path: sample, w: 640, h: 360, duration: 10, track: 2, start: 1, in: 0, out: 3, x: 160,
    y: 90, scale: 0.5, opacity: 100, visible: true, locked: false }],
}, null, 2));
w.check(fs.existsSync(file), 'an AvLoop project of version 3 written', path.basename(file));
w.verdict();
