// AV-002's import walk (S2, spec §3.9): every sample opens — or says why not —, its first, middle and last frames
// decode; the special cases come out right: upright rotation (against ffmpeg's own autorotate), square pixels,
// deinterlaced (less combing than the raw fields), tone-mapped HDR, alpha kept, 4K scaled to 1920; a Bengali path
// works; hardware and software frames agree.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const SAMPLES = path.join(lab.REPO, 'samples');
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-import-'));
const KEEP = new Set(['rotate90.mp4', 'sar43.mp4', 'interlaced.mpg', 'hdr-pq.mkv', 'alpha-vp9.webm', 'prores4444.mov', 'uhd-4k.mp4']);

const pngSize = (f) => { const b = fs.readFileSync(f); return [b.readUInt32BE(16), b.readUInt32BE(20)]; };
const rgbaOf = (png) => {
  const [pw, ph] = pngSize(png);
  const r = lab.run(lab.ffmpegExe, ['-v', 'error', '-i', png, '-f', 'rawvideo', '-pix_fmt', 'rgba', '-'], { raw: true });
  return { w: pw, h: ph, rgba: r.stdout };
};
// Combing: the mean difference of neighbouring rows against rows two apart (fields) — high when the fields differ.
const combing = ({ w: cw, h: ch, rgba }) => {
  let near = 0; let far = 0;
  for (let y = 0; y + 2 < ch; y += 2) for (let x = 0; x < cw; x += 2) {
    const i = (y * cw + x) * 4;
    near += Math.abs(rgba[i + 1] - rgba[i + 1 + cw * 4]);
    far += Math.abs(rgba[i + 1] - rgba[i + 1 + cw * 8]);
  }
  return near / Math.max(1, far);
};

const videos = [];
const walkDir = (d) => { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const p = path.join(d, e.name); if (e.isDirectory()) walkDir(p); else videos.push(p); } };
walkDir(SAMPLES);
let opened = 0;
for (const file of videos.sort()) {
  const name = path.relative(SAMPLES, file).replace(/\\/g, '/');
  const probe = lab.cli(['probe', file]);
  if (/^(audio-only\.m4a|broken\.mp4|family-anim\.webp)$/.test(name)) {
    w.check(probe.status !== 0 && /ERROR /.test(probe.out), `refused ${name}`, probe.out.trim().split('\n').pop());
    continue;
  }
  if (probe.status !== 0) { w.check(false, `opens ${name}`, probe.out.trim()); continue; }
  const info = JSON.parse(probe.out);
  const last = Math.max(0, Math.round(info.frameCount) - 1);
  const shots = [];
  for (const [label, at] of [['first', 0], ['middle', Math.floor(last / 2)], ['last', last]]) {
    const out = path.join(KEEP.has(name) ? HERE : TMP, `${name.replace(/[^\w.-]/g, '_')}-${label}.png`);
    const r = lab.cli(['frame', file, '--at', String(at), '-o', out]);
    shots.push({ label, out, ok: r.status === 0 && fs.existsSync(out), line: r.out.trim().split('\n')[0] });
  }
  const bad = shots.filter((s) => !s.ok);
  const [fw, fh] = shots[0].ok ? pngSize(shots[0].out) : [0, 0];
  if (w.check(bad.length === 0, `opens ${name}`, `${info.status} · ${info.codec} in ${info.container} · frames 0/${Math.floor(last / 2)}/${last} → ${fw}×${fh}${bad.length ? ` · ${bad.map((s) => s.line).join('; ')}` : ''}`)) opened++;
}
w.note(`${opened} files opened, their first, middle and last frames decoded`);

// The special cases, read from the kept pictures.
const at = (n, l = 'first') => path.join(HERE, `${n}-${l}.png`);
const rot = path.join(TMP, 'ffmpeg-rotate.png');
lab.run(lab.ffmpegExe, ['-v', 'error', '-y', '-i', path.join(SAMPLES, 'rotate90.mp4'), '-frames:v', '1', rot]);
const ours = rgbaOf(at('rotate90.mp4'));
const theirs = rgbaOf(rot);
const dRot = ours.w === theirs.w && ours.h === theirs.h ? lab.meanDiff(ours.rgba, theirs.rgba) : { mean: NaN, max: NaN };
w.check(ours.w === 360 && ours.h === 640 && dRot.mean <= 2, 'rotation upright (vs ffmpeg autorotate)', `${ours.w}×${ours.h}, mean difference ${dRot.mean.toFixed(3)}`);
const [sw, sh] = pngSize(at('sar43.mp4'));
w.check(sw === 854 && sh === 480, 'SAR 4:3 to square pixels', `${sw}×${sh}`);
// A reference frame by ffmpeg itself (`-vf <chain>`), and the same frame without the chain.
const reference = (file, n, chain) => {
  const out = path.join(TMP, `ref-${chain ? 'with' : 'raw'}-${path.basename(file)}.png`);
  const vf = [chain, `select=eq(n\\,${n})`].filter(Boolean).join(',');
  lab.run(lab.ffmpegExe, ['-v', 'error', '-y', '-i', file, '-vf', vf, '-frames:v', '1', out]);
  return rgbaOf(out);
};
const il = path.join(SAMPLES, 'interlaced.mpg');
const ilOurs = rgbaOf(at('interlaced.mpg', 'middle'));
const ilBwdif = lab.meanDiff(ilOurs.rgba, reference(il, 24, 'bwdif=mode=send_frame:parity=auto:deint=interlaced').rgba);
const ilRaw = lab.meanDiff(ilOurs.rgba, reference(il, 24, '').rgba);
w.check(ilBwdif.mean <= 2 && ilRaw.mean > ilBwdif.mean, 'deinterlaced (bwdif)',
  `frame 24 against ffmpeg's bwdif: mean ${ilBwdif.mean.toFixed(2)}; against the woven fields: ${ilRaw.mean.toFixed(2)}; combing ${combing(ilOurs).toFixed(3)}`);
const hdrFile = path.join(SAMPLES, 'hdr-pq.mkv');
const hdrOurs = rgbaOf(at('hdr-pq.mkv'));
const hdrChain = 'zscale=t=linear:npl=100,format=gbrpf32le,zscale=p=bt709,tonemap=tonemap=hable:desat=0,zscale=t=bt709:m=bt709:r=tv,format=yuv420p';
const hdrMapped = lab.meanDiff(hdrOurs.rgba, reference(hdrFile, 0, hdrChain).rgba);
const hdrNaive = lab.meanDiff(hdrOurs.rgba, reference(hdrFile, 0, '').rgba);
w.check(hdrMapped.mean <= 2 && hdrNaive.mean > hdrMapped.mean, 'HDR tone-mapped to SDR (zscale + hable)',
  `against ffmpeg's tone-mapped frame: mean ${hdrMapped.mean.toFixed(2)}; against the untone-mapped decode: ${hdrNaive.mean.toFixed(2)}`);
for (const n of ['alpha-vp9.webm', 'prores4444.mov']) {
  const corner = lab.pixelAt(at(n), 2, 2)[3];
  const [aw, ah] = pngSize(at(n));
  const centre = lab.pixelAt(at(n), aw >> 1, ah >> 1)[3];
  w.check(corner === 0 && centre === 255, `alpha kept: ${n}`, `corner alpha ${corner}, centre alpha ${centre}`);
}
const [kw, kh] = pngSize(at('uhd-4k.mp4'));
w.check(kw === 1920 && kh === 1080, '4K scaled to the working size', `${kw}×${kh}`);
const hw = path.join(TMP, 'hw.png');
const sw2 = path.join(TMP, 'sw.png');
const rh = lab.cli(['frame', path.join(SAMPLES, 'counter-1080p30.mp4'), '--at', '200', '-o', hw]);
lab.cli(['frame', path.join(SAMPLES, 'counter-1080p30.mp4'), '--at', '200', '--hw', 'off', '-o', sw2]);
const dHw = lab.meanDiff(rgbaOf(hw).rgba, rgbaOf(sw2).rgba);
w.check(/ hw /.test(rh.out) && dHw.mean <= 1.0, 'HW decode on and off give the same frame', `${rh.out.includes(' hw ') ? 'D3D11VA used' : 'no hardware'}; mean ${dHw.mean.toFixed(3)}, max ${dHw.max}`);
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
