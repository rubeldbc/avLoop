// AV-001's walk (S1 and the licence gate): avloop-cli's version line; the second start under 2 s; the window pictured
// in both themes at 100, 150 and 200% display scale; each picture's size read back.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();

const version = lab.cli(['version']);
const licence = version.out.match(/^licence: (.*)$/m)?.[1] ?? '?';
w.check(version.status === 0 && /PASS: LGPL build/.test(version.out), 'avloop-cli version (no --enable-gpl)', licence);

const starts = [];
for (let i = 0; i < 2; i++) {
  const file = path.join(HERE, `start-${i + 1}.png`);
  const began = Date.now();
  const r = lab.app(['--shot', file]);
  const ms = Number(r.out.match(/startup: (\d+) ms/)?.[1] ?? NaN);
  starts.push({ ms, wall: Date.now() - began });
  fs.rmSync(file, { force: true });
}
w.check(starts[1].ms < 2000, 'second start: window shown', `${starts[1].ms} ms (first ${starts[0].ms} ms; process ${starts[1].wall} ms with the 800 ms settle)`);

const pngSize = (file) => {
  const b = fs.readFileSync(file);
  return [b.readUInt32BE(16), b.readUInt32BE(20)];
};
for (const theme of ['navy', 'black']) {
  for (const scale of ['1', '1.5', '2']) {
    const file = path.join(HERE, `s1-${theme}-${Math.round(Number(scale) * 100)}.png`);
    const r = lab.app(['--shot', file, '--theme', theme, '--scale', scale]);
    const ok = r.status === 0 && fs.existsSync(file);
    const [pw, ph] = ok ? pngSize(file) : [0, 0];
    w.check(ok, `shot ${path.basename(file)}`, `${pw}×${ph} px, ${ok ? Math.round(fs.statSync(file).size / 1024) : 0} KB`);
    if (ok) {
      const bg = lab.pixelAt(file, Math.round(pw * 0.45), Math.round(ph * 0.25));
      w.note(`canvas background at 45%,25%: rgb(${bg.slice(0, 3).join(', ')})`);
    }
  }
}
w.verdict();
