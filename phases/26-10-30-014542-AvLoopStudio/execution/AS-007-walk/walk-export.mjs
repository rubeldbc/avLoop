// AS-007's export walk (D-037): a sequence made by hand (version 5) — V1 the counter 0–1 s, then the counter from its
// 5 s (a cut), an ellipse Cut on the second clip (a hole), V2 the 1080p counter at 25 % in a corner — exported In 0 →
// Out 2 s with the loop mode None (the sequence as cut) by avloop-cli in every format: WebM VP9 and VP8, APNG, MOV
// ProRes 4444 (alpha), GIF (1-bit), MP4 (the Background behind). Each file read back: codec, size, 60 frames, the
// alpha by the alpha-keeping decoder — Auto on (the hole); the cut's frames (the counter's 15 at 0.5 s, its 165 at
// 1.5 s, the hole clear) and V2 seen; no audio.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-studio-export-'));
const c640 = path.join(lab.REPO, 'samples', 'counter-640p30.mp4').split(path.sep).join('/');
const c1080 = path.join(lab.REPO, 'samples', 'counter-1080p30.mp4').split(path.sep).join('/');
const clip = (id, extra) => ({ id, name: 'counter', path: c640, w: 640, h: 360, duration: 10, start: 0, in: 0, out: 10,
  x: 320, y: 180, scale: 1, flipH: false, flipV: false, keys: [], opacity: 100, effects: {}, masks: [], enabled: true,
  ...extra });
const hole = { kind: 'ellipse', role: 'cut', name: 'Hole', visible: true, locked: false, feather: 0, strength: 100,
  x: 220, y: 130, w: 200, h: 100, radius: 0 };
const project = path.join(TMP, 'cut.avloop');
fs.writeFileSync(project, JSON.stringify({
  format: 'avloop', version: 5, source: { path: c640, stream: 0 }, range: { in: 0, out: 2 },
  transform: { x: 320, y: 180, scale: 1 }, loop: { mode: 'none' },
  output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: '30/1', background: '#ff00ff' },
  sequence: { x: 0, y: 0, w: 640, h: 360, fps: '30/1', in: 0, out: 2, main: 1, nextId: 4, matte: {}, tracks: [
    { name: 'V1', visible: true, locked: false, clips: [clip(1, { out: 1 }), clip(2, { start: 1, in: 5, out: 10, masks: [hole] })] },
    { name: 'V2', visible: true, locked: false, clips: [clip(3, { path: c1080, w: 1920, h: 1080, x: 560, y: 300, scale: 0.125 })] },
  ] },
}));

function counter(f, x0 = 0, width = f.w, foot = 16) { // the counter's 16 stripes (tests/Samples.h's reading)
  let n = 0;
  for (let k = 0; k < 16; k++) {
    const x = x0 + Math.floor(k * width / 16 + width / 32), y = f.h - foot;
    const p = f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 3);
    if ((p[0] + p[1] + p[2]) / 3 > 128) n |= 1 << k;
  }
  return n;
}
const alphaAt = (f, x, y) => f.rgba[(y * f.w + x) * 4 + 3];
const rgbAt = (f, x, y) => [...f.rgba.subarray((y * f.w + x) * 4, (y * f.w + x) * 4 + 3)];

const cases = [
  { id: 'webm-vp9', ext: 'webm', codec: 'vp9', alpha: 'full' },
  { id: 'webm-vp8', ext: 'webm', codec: 'vp8', alpha: 'full' },
  { id: 'apng', ext: 'apng', codec: 'apng', alpha: 'full' },
  { id: 'mov-prores', ext: 'mov', codec: 'prores', alpha: 'full' },
  { id: 'gif', ext: 'gif', codec: 'gif', alpha: 'bit' },
  { id: 'mp4-h264', ext: 'mp4', codec: 'h264', alpha: 'none' },
];
for (const c of cases) {
  const out = path.join(TMP, `cut-${c.id}.${c.ext}`);
  const r = lab.cli(['export', project, '-o', out, '--format', c.id]);
  const ok = r.status === 0 && fs.existsSync(out);
  if (!w.check(ok, `${c.id}: exported and checked by the app`, r.out.trim().split(/\r?\n/).pop().split(TMP).join('…'))) continue;
  const p = lab.probe(out);
  const v = lab.videoOf(p);
  w.check(v.codec_name === c.codec && v.width === 640 && v.height === 360 && Number(v.nb_read_frames) === 60 &&
    p.streams.length === 1, `${c.id}: ${c.codec} 640×360, 60 frames (In 0 → Out 2 s, as cut), no audio`,
    `${v.codec_name} ${v.width}×${v.height} ${v.nb_read_frames} frames ${v.pix_fmt}`);
  // The foot's stripes left of V2 (x < 440): the low 11 bits.
  const a = lab.frameAt(out, 15), b = lab.frameAt(out, 45);
  const low = (f) => counter(f) & 0x7ff;
  w.check(Math.abs(low(a) - 15) <= 1 && Math.abs(low(b) - 165) <= 1, `${c.id}: the cut — frame 15 at 0.5 s, the counter's 165 at 1.5 s`,
    `${low(a)} | ${low(b)}`);
  if (c.alpha === 'none') {
    const bg = rgbAt(b, 320, 180);
    w.check(bg[0] > 220 && bg[1] < 40 && bg[2] > 220, `${c.id}: no alpha — the hole the Background (#ff00ff)`, bg.join(','));
  } else {
    w.check(alphaAt(b, 320, 180) === 0 && alphaAt(b, 40, 300) === 255 && alphaAt(a, 320, 180) === 255,
      `${c.id}: Auto alpha — the hole clear at 1.5 s, the rest opaque; no hole at 0.5 s`,
      `${alphaAt(b, 320, 180)} | ${alphaAt(b, 40, 300)} | ${alphaAt(a, 320, 180)}`);
  }
  // V2's 1080p counter at 12.5 % (240 × 135, from 440, 232.5) over the corner: not V1's colours there.
  const v2 = rgbAt(a, 470, 250), v1 = rgbAt(a, 300, 250);
  w.check(v2.join() !== v1.join(), `${c.id}: V2 seen in its corner`, `${v2} vs ${v1}`);
  if (c.id === 'webm-vp9') fs.copyFileSync(out, path.join(TMP, 'keep.webm'));
}
const shot = lab.run(lab.ffmpegExe, ['-v', 'error', '-c:v', 'libvpx-vp9', '-i', path.join(TMP, 'keep.webm'), '-vf',
  'select=eq(n\\,45)', '-frames:v', '1', '-y', path.join(HERE, 'export-vp9-1.5s.png')]);
w.check(shot.status === 0, 'the VP9 file\'s frame at 1.5 s kept as a picture', 'export-vp9-1.5s.png');
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
