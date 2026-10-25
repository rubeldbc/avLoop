// AV-003's DPI walk: the canvas with a video at 100% and 150% display scale — the frame, the checkerboard, the
// Selection's dashed border and handles, the options bar — each pictured and its size read back.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-dpi-'));
const script = path.join(TMP, 'dpi.json');
for (const scale of ['1', '1.5']) {
  const file = `canvas-${Math.round(Number(scale) * 100)}.png`;
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 700, log: false, steps: [
    { do: 'open', file: 'samples/alpha-vp9.webm' },
    { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 10000 },
    { do: 'wait', ms: 400 },
    { do: 'expect', probe: 'canvas.gl', eq: true },
    { do: 'shot', file },
  ] }));
  const r = lab.app(['--walk', script, '--walk-out', HERE, '--scale', scale]);
  const png = path.join(HERE, file);
  const ok = r.status === 0 && fs.existsSync(png);
  const b = ok ? fs.readFileSync(png) : null;
  w.check(ok, `canvas at ${scale * 100}%`, ok ? `${b.readUInt32BE(16)}×${b.readUInt32BE(20)} px, ${Math.round(b.length / 1024)} KB` : r.out.trim().split('\n').pop());
}
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
