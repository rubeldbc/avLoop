// AvLoop's own lab kit (spec §2.6; control/04 §1): the built binaries, ffprobe/ffmpeg readings of what AvLoop wrote,
// the seam's difference, a picture's pixel, and the check that a walk left nothing behind. Node built-ins only.
// ffmpeg and ffprobe: the fetched LGPL build (third_party/ffmpeg/bin) first, else the ones on PATH. Every command
// carries a five-minute ceiling (spec §2.6).
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
export const REPO = path.resolve(HERE, '..', '..', '..', '..');
export const TIMEOUT = 300000;
const FFBIN = path.join(REPO, 'third_party', 'ffmpeg', 'bin');
const tool = (name) => (fs.existsSync(path.join(FFBIN, `${name}.exe`)) ? path.join(FFBIN, `${name}.exe`) : name);
export const ffmpegExe = tool('ffmpeg');
export const ffprobeExe = tool('ffprobe');

/** The built binary: build/bin/<name>.exe (scripts/build.ps1's layout). */
export function bin(name) {
  return path.join(REPO, 'build', 'bin', `${name}.exe`);
}

/** Runs a command; returns { status, out, err, stdout (Buffer when raw) }. */
export function run(cmd, args, { raw = false, timeout = TIMEOUT, cwd = REPO, env } = {}) {
  const r = spawnSync(cmd, args, {
    cwd, timeout, maxBuffer: 1 << 30, encoding: raw ? 'buffer' : 'utf8', env: env ? { ...process.env, ...env } : process.env,
    windowsHide: true,
  });
  const text = (b) => (b == null ? '' : Buffer.isBuffer(b) ? b.toString('utf8') : b);
  return { status: r.status, signal: r.signal, error: r.error, out: raw ? '' : text(r.stdout), err: text(r.stderr), stdout: r.stdout };
}

export const cli = (args, opts) => run(bin('avloop-cli'), args, opts);
export const app = (args, opts) => run(bin('avloop'), args, opts);

/** ffprobe's JSON of a file: format and streams (spec §2.6 `probe`). */
export function probe(file) {
  const r = run(ffprobeExe, ['-v', 'error', '-show_format', '-show_streams', '-count_frames', '-of', 'json', file]);
  if (r.status !== 0) throw new Error(`ffprobe failed on ${file}: ${r.err.trim()}`);
  return JSON.parse(r.out);
}

export const videoOf = (p) => p.streams.find((s) => s.codec_type === 'video');

/** The decoder that keeps alpha: libvpx for VP8/VP9 (the native vp9 decoder drops it, spec §3.8). */
function decoderArgs(file) {
  const v = videoOf(probe(file));
  if (v?.codec_name === 'vp9') return ['-c:v', 'libvpx-vp9'];
  if (v?.codec_name === 'vp8') return ['-c:v', 'libvpx'];
  return [];
}

/** Frame n of a file as RGBA: { w, h, rgba }, decoded with the alpha-keeping decoder. */
export function frameAt(file, n = 0, { scale } = {}) {
  const v = videoOf(probe(file));
  const filters = [`select=eq(n\\,${n})`];
  let w = v.width;
  let h = v.height;
  if (scale) { filters.push(`scale=${scale[0]}:${scale[1]}`); [w, h] = scale; }
  const r = run(ffmpegExe, ['-v', 'error', ...decoderArgs(file), '-i', file, '-vf', filters.join(','), '-frames:v', '1',
    '-f', 'rawvideo', '-pix_fmt', 'rgba', '-'], { raw: true });
  if (r.status !== 0 || r.stdout.length !== w * h * 4) throw new Error(`frameAt ${file} #${n}: ${r.err.trim()}`);
  return { w, h, rgba: r.stdout };
}

/** Alpha of the first frame by the libvpx decoder (spec §2.6 `alphaOf`): min, max, mean, corner and centre. */
export function alphaOf(file) {
  const { w, h, rgba } = frameAt(file, 0);
  let min = 255;
  let max = 0;
  let sum = 0;
  for (let i = 3; i < rgba.length; i += 4) { const a = rgba[i]; if (a < min) min = a; if (a > max) max = a; sum += a; }
  const at = (x, y) => rgba[(y * w + x) * 4 + 3];
  return { w, h, min, max, mean: sum / (w * h), corner: at(0, 0), centre: at(w >> 1, h >> 1) };
}

/** Every frame of a file, scaled small, as RGBA buffers (for differences only). */
export function framesSmall(file, size = 128) {
  const r = run(ffmpegExe, ['-v', 'error', ...decoderArgs(file), '-i', file, '-vf', `scale=${size}:${size}`, '-f', 'rawvideo',
    '-pix_fmt', 'rgba', '-'], { raw: true });
  if (r.status !== 0) throw new Error(`framesSmall ${file}: ${r.err.trim()}`);
  const n = size * size * 4;
  const frames = [];
  for (let at = 0; at + n <= r.stdout.length; at += n) frames.push(r.stdout.subarray(at, at + n));
  return frames;
}

/** Mean absolute difference per channel (0…255) of two equal-sized RGBA buffers, and its maximum. */
export function meanDiff(a, b) {
  if (a.length !== b.length) throw new Error('meanDiff: sizes differ');
  let sum = 0;
  let max = 0;
  for (let i = 0; i < a.length; i++) { const d = Math.abs(a[i] - b[i]); sum += d; if (d > max) max = d; }
  return { mean: sum / a.length, max };
}

/** The seam (spec §2.6 `seamDiff`, S7): last→first difference against the median of neighbouring frames' differences. */
export function seamDiff(file) {
  const frames = framesSmall(file);
  if (frames.length < 2) throw new Error(`seamDiff ${file}: ${frames.length} frame(s)`);
  const steps = [];
  for (let i = 1; i < frames.length; i++) steps.push(meanDiff(frames[i - 1], frames[i]).mean);
  const sorted = [...steps].sort((x, y) => x - y);
  const median = sorted[Math.floor(sorted.length / 2)];
  const seam = meanDiff(frames[frames.length - 1], frames[0]).mean;
  const duplicates = steps.filter((s) => s === 0).length;
  return { frames: frames.length, seam, median, ratio: median > 0 ? seam / median : seam === 0 ? 0 : Infinity, duplicates };
}

/** One pixel of a picture (spec §2.6 `pixelAt`): [r, g, b, a]. */
export function pixelAt(png, x, y) {
  const r = run(ffmpegExe, ['-v', 'error', '-i', png, '-vf', `crop=1:1:${x}:${y}`, '-frames:v', '1', '-f', 'rawvideo',
    '-pix_fmt', 'rgba', '-'], { raw: true });
  if (r.status !== 0 || r.stdout.length !== 4) throw new Error(`pixelAt ${png} ${x},${y}: ${r.err.trim()}`);
  return [...r.stdout];
}

/** Processes of AvLoop still running (a walk must leave none). */
export function avloopProcesses() {
  const r = run('tasklist', ['/FO', 'CSV', '/NH']);
  return r.out.split(/\r?\n/).filter((l) => /^"avloop(-cli)?\.exe"/i.test(l)).map((l) => l.split('","')[0].replace(/"/g, ''));
}

/** Session cache folders still on disk (spec §3.5: deleted on exit), under the disk cache's default root: Qt's
 * GenericCacheLocation (%LOCALAPPDATA%\cache) + AvLoop/cache (DiskCache::defaultRoot). */
export function cacheSessions() {
  const dir = path.join(process.env.LOCALAPPDATA ?? os.tmpdir(), 'cache', 'AvLoop', 'cache');
  return fs.existsSync(dir) ? fs.readdirSync(dir) : [];
}

/** A walk's verdict helpers: `check(ok, what, observed)` prints PASS/FAIL lines; `verdict()` prints WALK PASSED/FAILED. */
export function walkLog() {
  let failed = 0;
  return {
    check(ok, what, observed) { console.log(`${ok ? 'PASS' : 'FAIL'} ${what}: ${observed}`); if (!ok) failed++; return ok; },
    note(text) { console.log(`     ${text}`); },
    verdict() { console.log(failed ? `WALK FAILED (${failed})` : 'WALK PASSED'); process.exitCode = failed ? 1 : 0; return failed; },
  };
}
