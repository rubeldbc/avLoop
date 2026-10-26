// AV-013's formats walk (spec §10.4, S14): `avloop-cli export --format` — the app's ExportJob and its check — of the
// counter sample with an ellipse crop, read back by ffprobe: GIF (palettegen + paletteuse, 1-bit alpha, loop for ever),
// APNG (full alpha, plays for ever), MP4 H.264 by Media Foundation (no alpha: the Background colour behind, faststart),
// MOV ProRes 4444 (yuva444p, full alpha); codec, W×H, frame count, alpha; GIF at most 50 fps.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, writeProject, exportProject, packets } = await import(
  pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-formats-'));
const video = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const ellipse = { kind: 'ellipse', x: 120, y: 40, w: 400, h: 280, feather: 8 };
const project = writeProject(path.join(TMP, 'a.avloop'), video,
  { crop: ellipse, output: { background: '#ff00ff' } });
const L = 90; // 2 s – 5 s at 30 fps, Simple

const cases = [
  { id: 'gif', ext: 'gif', codec: 'gif', alpha: 'bit', note: 'GIF' },
  { id: 'apng', ext: 'apng', codec: 'apng', alpha: 'full', note: 'APNG' },
  { id: 'mp4-h264', ext: 'mp4', codec: 'h264', alpha: 'none', note: 'MP4 H.264' },
  { id: 'mov-prores', ext: 'mov', codec: 'prores', alpha: 'full', note: 'MOV ProRes 4444' },
];
const files = {};
for (const c of cases) {
  const out = path.join(TMP, `loop.${c.ext}`);
  const r = exportProject(project, out, ['--format', c.id]);
  if (!w.check(r.ok, `${c.note}: exported and checked by the app`, r.text)) continue;
  files[c.id] = out;
  const p = lab.probe(out);
  const v = lab.videoOf(p);
  w.check(v.codec_name === c.codec && v.width === 400 && v.height === 280, `${c.note}: codec and W×H`,
    `${v.codec_name} ${v.width}×${v.height} ${v.pix_fmt}`);
  w.check(Number(v.nb_read_frames) === L && r.frames === L, `${c.note}: frame count = L`,
    `${v.nb_read_frames} frames read, L = ${r.frames}`);
  w.check(!p.streams.some((s) => s.codec_type !== 'video'), `${c.note}: no audio stream`,
    p.streams.map((s) => s.codec_type).join(', '));
  const a = lab.alphaOf(out);
  if (c.alpha === 'none') {
    const corner = lab.frameAt(out, 0).rgba.subarray(0, 4);
    w.check(a.min === 255 && corner[0] > 230 && corner[1] < 25 && corner[2] > 230,
      `${c.note}: no alpha, the Background colour (#ff00ff) behind`, `alpha min ${a.min}, corner rgb ${[...corner].slice(0, 3)}`);
  } else {
    // 1-bit: only 0 and 255; full: the feathered edge keeps the values between.
    const { rgba } = lab.frameAt(out, 0);
    let partial = 0;
    for (let i = 3; i < rgba.length; i += 4) if (rgba[i] > 0 && rgba[i] < 255) partial++;
    w.check(a.corner === 0 && a.centre === 255 && (c.alpha === 'bit' ? partial === 0 : partial > 100),
      `${c.note}: alpha ${c.alpha === 'bit' ? '1-bit' : 'full'}`, `corner ${a.corner}, centre ${a.centre}, ${partial} partial pixels`);
  }
}

// What each container says of its loop: GIF's NETSCAPE2.0 loop count 0, APNG's acTL plays 0, MP4's moov first,
// ProRes's profile 4444.
if (files.gif) {
  const b = fs.readFileSync(files.gif);
  const at = b.indexOf('NETSCAPE2.0');
  const loops = at >= 0 ? b.readUInt16LE(at + 13) : -1;
  w.check(loops === 0, 'GIF: -loop 0 (for ever)', at >= 0 ? `NETSCAPE2.0 loop count ${loops}` : 'no NETSCAPE2.0 block');
}
if (files.apng) {
  const b = fs.readFileSync(files.apng);
  const at = b.indexOf('acTL');
  w.check(at >= 0 && b.readUInt32BE(at + 4) === L && b.readUInt32BE(at + 8) === 0, 'APNG: -plays 0 (for ever)',
    at >= 0 ? `acTL frames ${b.readUInt32BE(at + 4)}, plays ${b.readUInt32BE(at + 8)}` : 'no acTL');
}
if (files['mp4-h264']) {
  const b = fs.readFileSync(files['mp4-h264']);
  const moov = b.indexOf('moov');
  const mdat = b.indexOf('mdat');
  w.check(moov > 0 && moov < mdat, 'MP4: +faststart (moov before mdat)', `moov at ${moov}, mdat at ${mdat}`);
  const r = exportProject(project, path.join(TMP, 'enc.mp4'), ['--format', 'mp4-h264']);
  w.check(/h264_mf/.test(r.text), 'MP4: encoded by Media Foundation (h264_mf), no GPL encoder', (/h264_mf|libopenh264/.exec(r.text) ?? ['no H.264 encoder named'])[0]);
}
if (files['mov-prores']) {
  const v = lab.videoOf(lab.probe(files['mov-prores']));
  w.check(/4444/.test(v.profile ?? '') && /^yuva444p/.test(v.pix_fmt), 'ProRes: profile 4444 with alpha',
    `profile ${v.profile}, ${v.pix_fmt}, tag ${v.codec_tag_string}`);
}

// GIF at most 50 fps: a project asking 60 fps writes 50 (delays of 2 hundredths).
const fast = writeProject(path.join(TMP, 'fast.avloop'), video, { crop: ellipse, output: { fps: '60/1' } });
const g = exportProject(fast, path.join(TMP, 'fast.gif'), ['--format', 'gif']);
const pk = g.ok ? packets(path.join(TMP, 'fast.gif')) : [];
const steps = new Set(pk.slice(1).map((x, i) => Math.round((x.t - pk[i].t) * 100)));
w.check(g.ok && g.fps === '50/1' && g.frames === 150 && pk.length === 150 && steps.size === 1 && steps.has(2),
  'GIF: 60 fps asked, 50 written', `${g.fps} fps, ${pk.length} frames, delays ${[...steps].join('/')} cs`);

fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
