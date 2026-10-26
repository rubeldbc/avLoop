// AV-009's walk helpers: a project file written for a sample (the owner's paths stay in the walk's temp folder) and
// `avloop-cli export` of it — the app's ExportJob, its rename and its check — read back as { ok, frames, size, text }.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
export const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);

/** A .avloop for `video` (absolute; 640×360 unless `size` says) with the given sections merged over a plain 2 s – 5 s
 * loop, the video at its initial place (centred, scale 1). */
export function writeProject(file, video, { range, crop, loop, output, quality, size = [640, 360] } = {}) {
  const project = {
    format: 'avloop', version: 1,
    source: { path: video.split(path.sep).join('/'), stream: 0 },
    range: range ?? { in: 2, out: 5 },
    transform: { x: size[0] / 2, y: size[1] / 2, scale: 1 },
    loop: { mode: 'simple', ...loop },
    output: { format: 'webm-vp9', size: 'crop', alpha: 'auto', fps: 'source', ...output },
    quality: { preset: 'medium', ...quality },
  };
  if (crop) project.crop = crop;
  fs.writeFileSync(file, JSON.stringify(project, null, 1));
  return file;
}

/** `avloop-cli export`: { ok, frames, size: [w, h], bytes, passes, text } from its PASS line. */
export function exportProject(project, out, extra = []) {
  const r = lab.cli(['export', project, '-o', out, ...extra]);
  // The temp folder's path names the owner: the logs keep the file names only.
  const text = r.out.split(/\r?\n/).filter((l) => /^(PASS|FAIL|moved|memory)/.test(l)).join(' | ')
    .split(path.dirname(out)).join('…');
  const m = /: (\d+)x(\d+), (\d+) frames at (\d+)\/(\d+), (\d+) bytes, (\d+) pass/.exec(r.out);
  return {
    ok: r.status === 0 && !!m,
    size: m ? [Number(m[1]), Number(m[2])] : null,
    frames: m ? Number(m[3]) : 0,
    fps: m ? `${m[4]}/${m[5]}` : '',
    bytes: m ? Number(m[6]) : 0,
    passes: m ? Number(m[7]) : 0,
    movedAside: /moved aside/.test(r.out),
    text: text || r.out.trim().split(/\r?\n/).pop(),
    out: r.out,
  };
}

/** Packets of the video stream with their key flags and times (ffprobe). */
export function packets(file) {
  const r = lab.run(lab.ffprobeExe, ['-v', 'error', '-select_streams', 'v:0', '-show_entries', 'packet=pts_time,flags',
    '-of', 'csv=p=0', file]);
  return r.out.trim().split(/\r?\n/).filter(Boolean).map((l) => { const [t, f] = l.split(','); return { t: Number(t), key: f.startsWith('K') }; });
}

/** Frames decoded by libvpx with the file read twice in a row (`-stream_loop 1`, S11): { frames, errors }. */
export function loopedTwice(file) {
  const v = lab.videoOf(lab.probe(file));
  const dec = v.codec_name === 'vp8' ? 'libvpx' : 'libvpx-vp9';
  const r = lab.run(lab.ffmpegExe, ['-v', 'error', '-stream_loop', '1', '-c:v', dec, '-i', file, '-vf', 'scale=16:16',
    '-f', 'rawvideo', '-pix_fmt', 'rgba', '-'], { raw: true });
  return { frames: r.stdout.length / (16 * 16 * 4), errors: r.err.trim() };
}
