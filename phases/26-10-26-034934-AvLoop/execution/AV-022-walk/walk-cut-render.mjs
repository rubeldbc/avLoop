// AV-022's render walk (D-020): Mode None — the clip as cut — in the export's own frames. The counter sample, In 1 s,
// Out 3 s (D = 60 at 30 fps): avloop-cli says a 60-frame output; frame 0, 30 and 59 are Simple's byte for byte (the
// same picks, no blend); frame 0 is not Crossfade's (which starts F frames in). The export: D frames (Crossfade's
// would be D − F), VP9, its first frame the frame at In; a version-2 file with "none" read back by the app's CLI.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, writeProject, exportProject } = await import(
  pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-cut-'));
const sample = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const D = 60;
const F = 15;

const project = (mode) =>
  writeProject(path.join(TMP, `${mode}.avloop`), sample, { range: { in: 1, out: 3 }, loop: { mode, fade: 0.5 },
    output: { fps: '30/1' } });
const projects = { none: project('none'), simple: project('simple'), crossfade: project('crossfade') };

function frame(name, k) {
  const raw = path.join(TMP, `${name}-${k}.rgba`);
  const r = lab.cli(['frame', projects[name], '--at', String(k), '-o', `${raw}.png`, '--raw', raw]);
  return { ok: r.status === 0, line: r.out.trim().split(/\r?\n/).pop() ?? '', bytes: r.status === 0 ? fs.readFileSync(raw) : null };
}
const same = (a, b) => a && b && a.length === b.length && a.equals(b);

const first = frame('none', 0);
w.check(first.ok && / of 60 /.test(first.line) && /· none ·/.test(first.line), 'None: a 60-frame output (L = D)',
  first.line.replace(/→.*$/, '').trim());
for (const k of [0, 30, D - 1]) {
  const a = k === 0 ? first : frame('none', k);
  const b = frame('simple', k);
  w.check(same(a.bytes, b.bytes), `None frame ${k} = Simple frame ${k}, byte for byte`, a.bytes ? `${a.bytes.length} bytes` : 'no frame');
}
const fade0 = frame('crossfade', 0);
w.check(fade0.ok && / of 45 /.test(fade0.line) && !same(first.bytes, fade0.bytes),
  'Crossfade starts F frames in, 45 frames: not None\'s', fade0.line.replace(/→.*$/, '').trim());
const atF = frame('none', F);
w.check(same(atF.bytes, fade0.bytes), 'None frame F = Crossfade frame 0 (S[F]): the same segment, cut', atF.bytes ? 'equal' : 'no frame');
fs.copyFileSync(path.join(TMP, 'none-0.rgba.png'), path.join(HERE, 'render-none-first.png'));

const out = path.join(TMP, 'none.webm');
const r = exportProject(projects.none, out);
w.check(r.ok, 'None: exported and checked', r.text);
w.check(r.frames === D, 'None: the file has D frames', `${r.frames} (D = ${D}; Crossfade's L would be ${D - F})`);
const v = r.ok ? lab.videoOf(lab.probe(out)) : null;
w.check(v && v.codec_name === 'vp9' && v.width === 640 && v.height === 360, 'ffprobe: vp9 640×360',
  v ? `${v.codec_name} ${v.width}x${v.height} r_frame_rate ${v.r_frame_rate}` : 'no stream');
if (r.ok) {
  // The encoded first frame against the rendered frames 0, 15 and 30: nearest to frame 0, the frame at In.
  const firstOut = lab.frameAt(out, 0, { scale: [160, 90] }).rgba;
  const diffs = [0, 15, 30].map((k) =>
    lab.meanDiff(firstOut, lab.frameAt(path.join(TMP, `none-${k}.rgba.png`), 0, { scale: [160, 90] }).rgba).mean);
  w.check(diffs[0] < diffs[1] && diffs[0] < diffs[2], 'the file\'s first frame is the frame at In',
    `mean difference to rendered frame 0 / 15 / 30: ${diffs.map((d) => d.toFixed(2)).join(' / ')}`);
}
const json = JSON.parse(fs.readFileSync(projects.none, 'utf8'));
w.check(json.loop.mode === 'none', 'the project file says "mode": "none"', JSON.stringify(json.loop));

fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
