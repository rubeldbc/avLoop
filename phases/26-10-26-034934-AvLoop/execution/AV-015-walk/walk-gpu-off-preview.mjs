// CP-A1's S13, second half (the Preview): S9 again with the GPU off (AVLOOP_GPU_RENDERING=off makes the app and
// avloop-cli render on the CPU whatever the settings say) — AV-008's walk and its check, as that phase wrote them.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-gpuoff-'));
const env = { ...process.env, AVLOOP_GPU_RENDERING: 'off' };
const EXEC = path.resolve(HERE, '..');

function walk(phase, file) {
  const script = path.join(EXEC, phase, file);
  const r = file.endsWith('.json')
    ? spawnSync(lab.bin('avloop'), ['--walk', script, '--walk-out', TMP], { cwd: lab.REPO, env, encoding: 'utf8', timeout: 295000 })
    : spawnSync(process.execPath, [script], { cwd: TMP, env, encoding: 'utf8', timeout: 295000 });
  const out = `${r.stdout ?? ''}`;
  const passes = out.split(/\r?\n/).filter((l) => l.startsWith('PASS')).length;
  const fails = out.split(/\r?\n/).filter((l) => l.startsWith('FAIL')).map((l) => l.split(TMP).join('…'));
  w.check(/^WALK PASSED/m.test(out), `${phase}/${file} with the GPU off`, fails.length ? fails.join(' | ') : `${passes} checks passed`);
  return out;
}

walk('AV-008-walk', 'walk-preview.json');
walk('AV-008-walk', 'walk-preview-check.mjs');
fs.rmSync(TMP, { recursive: true, force: true });
w.verdict();
