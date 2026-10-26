// AV-009's export walk (spec §10.1–§10.3, §12.3, S10): `avloop-cli export` — the app's ExportJob — of the counter
// sample, read back by ffprobe and the libvpx decoder: VP9 with Alpha Auto (on) — codec, W×H, alpha_mode=1, no audio,
// the first packet a keyframe, the alpha plane —; Alpha Off; VP8 the same; Target size within 5 % for VP9 and VP8; a
// destination held open (as OBS holds the file it plays) written beside it as `name (1).webm`; three exports in a row
// in the app with the memory flat (--mem-log). Cancel's "no .part" is walk-export-ui.json's and test-export-job's.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { lab, writeProject, exportProject, packets } from './export-lab.mjs';

const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-export-'));
const video = path.join(lab.REPO, 'samples', 'counter-640p30.mp4');
const ellipse = { kind: 'ellipse', x: 120, y: 40, w: 400, h: 280, feather: 8 };
const inside = { kind: 'rect', x: 120, y: 40, w: 400, h: 280 };

function check(name, project, { codec, size, alpha, corner = alpha ? 0 : 255 }) {
  const out = path.join(TMP, `${name}.webm`);
  const r = exportProject(project, out);
  if (!w.check(r.ok, `${name}: exported and checked by the app`, r.text)) return null;
  const p = lab.probe(out);
  const v = lab.videoOf(p);
  const tags = Object.fromEntries(Object.entries(v.tags ?? {}).map(([k, x]) => [k.toLowerCase(), x]));
  const pk = packets(out);
  const a = lab.alphaOf(out);
  w.check(v.codec_name === codec && v.width === size[0] && v.height === size[1],
    `${name}: codec and W×H`, `${v.codec_name} ${v.width}×${v.height}`);
  w.check((tags.alpha_mode === '1') === alpha, `${name}: alpha_mode ${alpha ? '= 1' : 'absent'}`,
    `alpha_mode ${tags.alpha_mode ?? 'absent'}`);
  w.check(!p.streams.some((s) => s.codec_type !== 'video'), `${name}: no audio stream`,
    p.streams.map((s) => s.codec_type).join(', '));
  const keys = pk.map((x, i) => (x.key ? i : -1)).filter((i) => i >= 0);
  w.check(pk[0]?.key === true && keys.length === 1, `${name}: the first packet a keyframe, one GOP`,
    `${pk.length} packets, keyframes at ${keys.join(', ')}`);
  w.check(pk.length === r.frames, `${name}: frame count = L`, `${pk.length} packets, L = ${r.frames}`);
  // The app's own check read the alpha plane through libvpx as well (its PASS line); here: clear corners, a solid centre.
  w.check(a.corner === corner && a.centre === 255 && (alpha || a.min === 255),
    `${name}: the alpha plane by libvpx`, `corner ${a.corner}, centre ${a.centre}, min ${a.min}`);
  return { out, ...r };
}

// 1. VP9: Alpha Auto with an ellipse (on), Off, On over an opaque crop, Auto over it (off); VP8 with alpha; Long side.
const vp9 = check('vp9-alpha', writeProject(path.join(TMP, 'a.avloop'), video, { crop: ellipse }),
  { codec: 'vp9', size: [400, 280], alpha: true });
check('vp9-off', writeProject(path.join(TMP, 'b.avloop'), video,
  { crop: ellipse, output: { alpha: 'off', background: '#ff00ff' } }), { codec: 'vp9', size: [400, 280], alpha: false });
check('vp9-on', writeProject(path.join(TMP, 'c.avloop'), video, { crop: inside, output: { alpha: 'on' } }),
  { codec: 'vp9', size: [400, 280], alpha: true, corner: 255 });
check('vp9-auto-opaque', writeProject(path.join(TMP, 'd.avloop'), video, { crop: inside }),
  { codec: 'vp9', size: [400, 280], alpha: false });
check('vp8-alpha', writeProject(path.join(TMP, 'e.avloop'), video, { crop: ellipse, output: { format: 'webm-vp8' } }),
  { codec: 'vp8', size: [400, 280], alpha: true });
check('vp9-long-256', writeProject(path.join(TMP, 'f.avloop'), video, { crop: ellipse, output: { size: 'long-256' } }),
  { codec: 'vp9', size: [256, 180], alpha: true });

// 2. Target size: two passes, within 5 % of the size asked for (a size below what the loop takes at its CRF).
for (const [format, mb] of [['webm-vp9', 0.1], ['webm-vp8', 0.12]]) {
  const project = writeProject(path.join(TMP, `t-${format}.avloop`), video,
    { crop: ellipse, output: { format }, quality: { preset: 'custom', crf: 31, speed: 2, targetMB: mb } });
  const r = exportProject(project, path.join(TMP, `target-${format}.webm`));
  const want = mb * 1048576;
  const off = (r.bytes - want) / want;
  w.check(r.ok && Math.abs(off) <= 0.05, `Target size ${mb} MB (${format})`,
    `${r.bytes} bytes, ${(off * 100).toFixed(1)} % off, ${r.passes} passes`);
}

// 3. A destination held open without sharing: five more tries a second apart, then `name (1).webm`.
const held = path.join(TMP, 'held.webm');
fs.writeFileSync(held, '');
const holder = spawn('powershell.exe', ['-NoProfile', '-Command',
  `$f = [System.IO.File]::Open('${held}', 'Open', 'Read', 'None'); Start-Sleep -Seconds 12; $f.Close()`],
{ windowsHide: true, stdio: 'ignore' });
await new Promise((resolve) => setTimeout(resolve, 1500));
const t0 = Date.now();
const moved = exportProject(path.join(TMP, 'a.avloop'), held);
const aside = path.join(TMP, 'held (1).webm');
w.check(moved.ok && moved.movedAside && fs.existsSync(aside) && fs.statSync(held).size === 0,
  'a held destination: written beside it',
  `${moved.text}; ${((Date.now() - t0) / 1000).toFixed(1)} s; held.webm ${fs.statSync(held).size} bytes`);
holder.kill();
w.check(!fs.readdirSync(TMP).some((f) => f.endsWith('.part')), 'no .part left anywhere',
  `${fs.readdirSync(TMP).filter((f) => f.endsWith('.webm')).length} files`);

// 4. Three exports in a row in the app: the memory flat (the third's peak against the second's).
const steps = [
  { do: 'open', file: 'samples/counter-640p30.mp4' },
  { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
  { do: 'type', on: 'transport.time', text: '1' }, { do: 'key', keys: 'I', on: 'MainWindow' },
  { do: 'type', on: 'transport.time', text: '8.9667' }, { do: 'key', keys: 'O', on: 'MainWindow' },
  { do: 'key', keys: 'E', on: 'MainWindow' }, { do: 'dclick', on: 'canvas', space: 'canvas', at: [320, 180] },
];
for (let i = 1; i <= 3; i++) {
  steps.push({ label: `export ${i}` }, { do: 'exportTo', file: path.join(TMP, `row-${i}.webm`) },
    { do: 'waitFor', probe: 'export.state', eq: 'done', timeout: 120000 }, { do: 'expect', probe: 'export.verify', eq: '' },
    { do: 'wait', ms: 2000 });
}
const script = path.join(TMP, 'three-exports.json');
fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 800, steps }));
const csv = path.join(TMP, 'memory-export.csv');
const app = lab.app(['--walk', script, '--walk-out', TMP, '--mem-log', csv]);
w.check(app.status === 0 && /WALK PASSED/.test(app.out), 'three exports in a row in the app', `exit ${app.status}`);
const rows = fs.readFileSync(csv, 'utf8').trim().split(/\r?\n/).slice(1).map((l) => l.split(',').map(Number));
const total = rows.length ? rows[rows.length - 1][0] : 0;
const third = (k, col) =>
  Math.max(...rows.filter((x) => x[0] >= (total * k) / 3 && x[0] < (total * (k + 1)) / 3).map((x) => x[col]));
const peaks = (col) => [0, 1, 2].map((k) => third(k, col).toFixed(0)).join(' / ');
w.check(third(2, 1) - third(1, 1) < 40, 'Private Bytes: export 3 against export 2', `peaks ${peaks(1)} MB`);
w.check(third(2, 2) - third(1, 2) < 32, 'VRAM: export 3 against export 2', `peaks ${peaks(2)} MB`);
fs.copyFileSync(csv, path.join(process.cwd(), 'memory-export.csv'));
if (vp9) w.note(`vp9-alpha: ${vp9.bytes} bytes for ${vp9.frames} frames`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
