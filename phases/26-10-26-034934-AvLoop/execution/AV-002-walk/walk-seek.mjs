// AV-002's seek walk (S3, first half): avloop-cli decodes 40 random frames of the counter sample, each from a fresh
// process (a cold seek every time); the counter read from each PNG is the frame asked for. The 200-seek check runs in
// test-decoder-seek (ctest) in software and with D3D11VA; this walk proves the same through the shipped binary.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-seek-'));
const video = path.join(lab.REPO, 'samples', 'counter-1080p30.mp4');

// The 16 stripes of the counter, read from the PNG's last 16th row.
const counterOf = (png) => {
  const r = lab.run(lab.ffmpegExe, ['-v', 'error', '-i', png, '-vf', 'crop=iw:1:0:ih-16', '-f', 'rawvideo', '-pix_fmt', 'gray', '-'], { raw: true });
  const row = r.stdout;
  let n = 0;
  for (let k = 0; k < 16; k++) if (row[Math.floor(k * row.length / 16 + row.length / 32)] > 128) n |= 1 << k;
  return n;
};

let seed = 2026;
const rand = (n) => { seed = (seed * 1103515245 + 12345) % 2147483648; return seed % n; };
let wrong = 0;
const asked = [];
let ms = 0;
for (let i = 0; i < 40; i++) {
  const n = rand(300);
  const png = path.join(TMP, `f${n}.png`);
  const r = lab.cli(['frame', video, '--at', String(n), '-o', png]);
  ms += Number(r.out.match(/ (\d+) ms /)?.[1] ?? 0);
  const got = r.status === 0 ? counterOf(png) : -1;
  if (got !== n) { wrong++; w.note(`asked ${n}, read ${got}: ${r.out.trim()}`); }
  asked.push(n);
}
w.check(wrong === 0, '40 cold seeks through avloop-cli (1080p H.264, D3D11VA)', `${40 - wrong}/40 exact; frames ${asked.slice(0, 12).join(', ')} …; mean decode ${Math.round(ms / 40)} ms a seek`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
