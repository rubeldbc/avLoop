// Makes the lab's sample videos into samples/ (spec §2.6, §3.9; control/04 §2), never committed. Each is small; the
// counter samples carry a 16-bit frame counter burned into testsrc2's foot as black/white stripes (bit k = stripe k,
// the least significant on the left) so a decoded frame says its own number.
// ffmpeg: the one on PATH (a lab tool) else third_party/ffmpeg/bin. A sample whose encoder this ffmpeg lacks is
// skipped and said so.
//   node tools/media/make-samples.mjs [--force] [name …]
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const OUT = path.join(ROOT, 'samples');
const args = process.argv.slice(2);
const force = args.includes('--force');
const only = args.filter((a) => !a.startsWith('--'));
const local = path.join(ROOT, 'third_party', 'ffmpeg', 'bin', 'ffmpeg.exe');
const onPath = spawnSync('ffmpeg', ['-hide_banner', '-version'], { encoding: 'utf8' }).status === 0;
const FFMPEG = onPath ? 'ffmpeg' : local;
const encoders = spawnSync(FFMPEG, ['-hide_banner', '-encoders'], { encoding: 'utf8' }).stdout ?? '';
const has = (e) => new RegExp(`^ \\S{6} ${e}\\s`, 'm').test(encoders);
const h264 = has('libx264') ? ['-c:v', 'libx264', '-crf', '14', '-preset', 'fast'] : ['-c:v', 'libopenh264', '-b:v', '8M'];

// A testsrc2 picture with the frame counter at its foot (32 px, 16 stripes).
const counter = (w, h, r, d, extra = '') => [
  '-f', 'lavfi', '-i', `testsrc2=s=${w}x${h}:r=${r}:d=${d}`,
  '-f', 'lavfi', '-i', `color=c=black:s=${w}x32:r=${r}:d=${d}`,
  '-filter_complex', `[1]format=gray,geq=lum='255*mod(floor(N/pow(2,floor(X*16/W))),2)'[b];[0][b]overlay=0:H-32:shortest=1,format=yuv420p${extra}`,
];
const src = (w = 320, h = 240, r = 25, d = 2) => ['-f', 'lavfi', '-i', `testsrc2=s=${w}x${h}:r=${r}:d=${d}`];
const alphaSrc = (w, h, r, d) => [
  '-f', 'lavfi', '-i', `testsrc2=s=${w}x${h}:r=${r}:d=${d}`,
  '-vf', "format=rgba,geq=r='r(X,Y)':g='g(X,Y)':b='b(X,Y)':a='if(lt(hypot(X-W/2,Y-H/2),H/3),255,0)'",
];

const SAMPLES = [
  // Seeking and playback (S3)
  { name: 'counter-640p30.mp4', need: [], args: [...counter(640, 360, 30, 10), ...h264, '-g', '60', '-bf', '2'] },
  { name: 'counter-1080p30.mp4', need: [], args: [...counter(1920, 1080, 30, 10), ...h264, '-g', '60', '-bf', '2'] },
  { name: 'counter-1080p60.mp4', need: [], args: [...counter(1920, 1080, 60, 10), ...h264, '-g', '120', '-bf', '2'] },
  // The source cases of spec §12.1 (S2)
  { name: 'rotate90.mp4', need: [], from: 'counter-640p30.mp4', args: ['-display_rotation', '90', '-i', '{from}', '-c', 'copy'] },
  { name: 'vfr.mp4', need: [], args: [...counter(640, 360, 30, 6, ",select='mod(n,5)'"), ...h264, '-fps_mode', 'vfr'] },
  { name: 'hdr-pq.mkv', need: ['libx265'], args: [...src(640, 360, 25, 3), '-vf', 'format=yuv420p10le', '-c:v', 'libx265', '-x265-params', 'colorprim=bt2020:transfer=smpte2084:colormatrix=bt2020nc:log-level=error', '-color_primaries', 'bt2020', '-color_trc', 'smpte2084', '-colorspace', 'bt2020nc'] },
  { name: 'interlaced.mpg', need: ['mpeg2video'], args: [...src(720, 576, 50, 2), '-vf', 'tinterlace=mode=interleave_top,setfield=tff', '-c:v', 'mpeg2video', '-flags', '+ilme+ildct', '-top', '1', '-b:v', '8M'] },
  { name: 'sar43.mp4', need: [], args: [...src(640, 480, 25, 2), '-vf', 'setsar=4/3', ...h264] },
  { name: 'alpha-vp9.webm', need: ['libvpx-vp9'], args: [...alphaSrc(640, 360, 30, 3), '-c:v', 'libvpx-vp9', '-pix_fmt', 'yuva420p', '-b:v', '2M'] },
  { name: 'prores4444.mov', need: ['prores_ks'], args: [...alphaSrc(640, 360, 25, 2), '-c:v', 'prores_ks', '-profile:v', '4444', '-pix_fmt', 'yuva444p10le'] },
  { name: 'uhd-4k.mp4', need: [], args: [...src(3840, 2160, 30, 2), ...h264] },
  { name: 'audio-only.m4a', need: ['aac'], args: ['-f', 'lavfi', '-i', 'sine=frequency=440:duration=2', '-c:a', 'aac'] },
  // The container and codec families of spec §3.9
  { name: 'family-h264.mkv', need: [], args: [...src(), ...h264] },
  { name: 'family-hevc.mp4', need: ['libx265'], args: [...src(), '-c:v', 'libx265', '-x265-params', 'log-level=error', '-tag:v', 'hvc1'] },
  { name: 'family-av1.mp4', need: ['libsvtav1'], args: [...src(), '-c:v', 'libsvtav1', '-preset', '10'] },
  { name: 'family-vp8.webm', need: ['libvpx'], args: [...src(), '-c:v', 'libvpx', '-b:v', '1M'] },
  { name: 'family-vp9.webm', need: ['libvpx-vp9'], args: [...src(), '-c:v', 'libvpx-vp9', '-b:v', '1M'] },
  { name: 'family-mpeg4.avi', need: ['mpeg4'], args: [...src(), '-c:v', 'mpeg4', '-q:v', '3'] },
  { name: 'family-mpeg1.mpg', need: ['mpeg1video'], args: [...src(), '-c:v', 'mpeg1video', '-b:v', '2M', '-f', 'mpeg'] },
  { name: 'family-mpeg2.vob', need: ['mpeg2video'], args: [...src(720, 576), '-c:v', 'mpeg2video', '-b:v', '4M', '-f', 'vob'] },
  { name: 'family-mpeg2.mpg', need: ['mpeg2video'], args: [...src(), '-c:v', 'mpeg2video', '-b:v', '2M', '-f', 'mpeg'] },
  { name: 'family-h264.ts', need: [], args: [...src(), ...h264, '-f', 'mpegts'] },
  { name: 'family-h264.m2ts', need: [], args: [...src(), ...h264, '-f', 'mpegts', '-mpegts_m2ts_mode', '1'] },
  { name: 'family-wmv2.wmv', need: ['wmv2'], args: [...src(), '-c:v', 'wmv2', '-b:v', '2M'] },
  { name: 'family-flv1.flv', need: ['flv'], args: [...src(), '-c:v', 'flv', '-b:v', '1M'] },
  { name: 'family-dnxhr.mxf', need: ['dnxhd'], args: [...src(1280, 720, 25, 1), '-c:v', 'dnxhd', '-profile:v', 'dnxhr_lb', '-pix_fmt', 'yuv422p'] },
  { name: 'family-h263.3gp', need: ['h263'], args: [...src(176, 144), '-c:v', 'h263', '-b:v', '256k'] },
  { name: 'family-theora.ogv', need: ['libtheora'], args: [...src(), '-c:v', 'libtheora', '-q:v', '7'] },
  { name: 'family-raw.y4m', need: ['rawvideo'], args: [...src(320, 240, 25, 1), '-pix_fmt', 'yuv420p', '-f', 'yuv4mpegpipe'] },
  { name: 'family-anim.gif', need: ['gif'], args: [...src(), '-vf', 'split[a][b];[a]palettegen[p];[b][p]paletteuse', '-loop', '0'] },
  { name: 'family-anim.apng', need: ['apng'], args: [...src(), '-c:v', 'apng', '-plays', '0', '-f', 'apng'] },
  { name: 'family-anim.webp', need: ['libwebp_anim'], args: [...src(), '-c:v', 'libwebp_anim', '-loop', '0'] },
  { name: 'family-mjpeg.avi', need: ['mjpeg'], args: [...src(), '-c:v', 'mjpeg', '-q:v', '3'] },
  { name: 'family-ffv1.mkv', need: ['ffv1'], args: [...src(), '-c:v', 'ffv1'] },
  { name: 'family-qtrle.mov', need: ['qtrle'], args: [...src(), '-c:v', 'qtrle', '-pix_fmt', 'argb'] },
  { name: 'family-png.mov', need: ['png'], args: [...src(), '-c:v', 'png'] },
  { name: 'family-raw.avi', need: ['rawvideo'], args: [...src(320, 240, 25, 1), '-c:v', 'rawvideo', '-pix_fmt', 'bgr24'] },
  { name: 'family-hap.mov', need: ['hap'], args: [...src(320, 240), '-c:v', 'hap'] },
  { name: 'family-cineform.mov', need: ['cfhd'], args: [...src(), '-c:v', 'cfhd'] },
  // The README's pictures: soft moving colours in AvLoop's palette.
  { name: 'readme-demo.mp4', need: [], args: ['-f', 'lavfi', '-i', 'gradients=s=1920x1080:r=30:d=10:n=3:c0=0x16223A:c1=0xFF7A1A:c2=0x3DD68C:speed=0.02', ...h264] },
  { name: 'family-prores.mov', need: ['prores_ks'], args: [...src(), '-c:v', 'prores_ks', '-profile:v', '2'] },
];

fs.mkdirSync(OUT, { recursive: true });
const run = (argv) => spawnSync(FFMPEG, ['-hide_banner', '-v', 'error', '-y', ...argv], { encoding: 'utf8', timeout: 300000 });
let made = 0;
let skipped = 0;
for (const s of SAMPLES) {
  if (only.length && !only.includes(s.name)) continue;
  const file = path.join(OUT, s.name);
  if (fs.existsSync(file) && !force) { console.log(`have    ${s.name}`); continue; }
  const missing = s.need.filter((e) => !has(e));
  if (missing.length) { console.log(`SKIP    ${s.name}: this ffmpeg has no ${missing.join(', ')}`); skipped++; continue; }
  const argv = s.args.map((a) => (a === '{from}' ? path.join(OUT, s.from) : a));
  const r = run([...argv, file]);
  if (r.status !== 0) { console.log(`FAIL    ${s.name}: ${(r.stderr || '').trim().split('\n').pop()}`); fs.rmSync(file, { force: true }); skipped++; continue; }
  console.log(`made    ${s.name} (${Math.round(fs.statSync(file).size / 1024)} KB)`);
  made++;
}

// Derived files: a Bengali path, a broken (truncated) file.
const base = path.join(OUT, 'counter-640p30.mp4');
if (fs.existsSync(base)) {
  const bengali = path.join(OUT, 'বাংলা নমুনা', 'ভিডিও.mp4');
  fs.mkdirSync(path.dirname(bengali), { recursive: true });
  if (!fs.existsSync(bengali) || force) fs.copyFileSync(base, bengali);
  const broken = path.join(OUT, 'broken.mp4');
  if (!fs.existsSync(broken) || force) {
    const bytes = fs.readFileSync(base);
    fs.writeFileSync(broken, bytes.subarray(0, Math.floor(bytes.length * 0.4)));
  }
  console.log('derived bengali path and broken.mp4');
}
console.log(`samples: ${made} made, ${skipped} skipped, ffmpeg: ${FFMPEG === 'ffmpeg' ? 'PATH' : 'third_party'}`);
