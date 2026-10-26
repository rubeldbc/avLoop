// AV-014's install walk (spec §1.2 item 14, S15): scripts\deploy.ps1 and ISCC make the installer; a silent per-user
// install into a temporary folder; the installed app starts with nothing but Windows on PATH (a picture); the .avloop
// association's command, read from the registry, opens a project (a picture); the silent uninstall leaves the settings
// and nothing else — the program, the association, the Start menu entry, the uninstall entry, the autosave, the logs
// and the disk cache gone. The owner's own AvLoop folders, if any, are kept aside and put back.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const { lab, writeProject } = await import(pathToFileURL(path.resolve(HERE, '..', 'AV-009-walk', 'export-lab.mjs')).href);
const w = lab.walkLog();
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-install-'));
const hide = (t) => t.split(TMP).join('…').split(os.homedir()).join('~');
// Inno Setup 6 for every user, or for this user alone (winget's per-user install, AV-017's PC).
const ISCC = [path.join(process.env['ProgramFiles(x86)'] ?? 'C:/Program Files (x86)', 'Inno Setup 6', 'ISCC.exe'),
  path.join(process.env.LOCALAPPDATA ?? '', 'Programs', 'Inno Setup 6', 'ISCC.exe')].find((p) => fs.existsSync(p)) ?? 'ISCC.exe';
const LOCAL = process.env.LOCALAPPDATA;
const ROAMING = process.env.APPDATA;
const owned = [path.join(LOCAL, 'AvLoop'), path.join(LOCAL, 'cache', 'AvLoop')]; // what uninstalling removes
const settings = path.join(ROAMING, 'AvLoop'); // what it keeps
// The Start menu entry: AV-017's name.
const startMenu = path.join(ROAMING, 'Microsoft', 'Windows', 'Start Menu', 'Programs', 'AvLoop by Rubel.lnk');
const reg = (key, value) => {
  const r = lab.run('reg', ['query', key, ...(value === undefined ? ['/ve'] : ['/v', value])]);
  const m = /REG_\w+\s+(.*)$/m.exec(r.out);
  return r.status === 0 ? (m ? m[1].trim() : '') : null;
};
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
// Windows alone on PATH (both spellings: Node passes the copy's keys as they are), no Qt plugin path.
const windowsOnly = `${process.env.SystemRoot}\\System32;${process.env.SystemRoot}`;
const env = { PATH: windowsOnly, Path: windowsOnly, QT_PLUGIN_PATH: '' };
// The uninstall entry: Inno's key from the installer's AppId.
const appId = /AppId=\{\{([0-9A-F-]+)\}/.exec(fs.readFileSync(path.join(lab.REPO, 'installer', 'AvLoop.iss'), 'utf8'))[1];
const uninstallKey = `HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{${appId}}_is1`;

// The owner's folders, kept aside; a marker in the settings folder (made if there is none).
const aside = owned.map((dir, i) => {
  if (!fs.existsSync(dir)) return null;
  const copy = path.join(TMP, `aside-${i}`);
  fs.cpSync(dir, copy, { recursive: true });
  return copy;
});
const settingsExisted = fs.existsSync(settings);
const marker = path.join(settings, 'walk-install-marker.txt');
fs.mkdirSync(settings, { recursive: true });
fs.writeFileSync(marker, 'kept by uninstalling (AV-014 walk)\n');

try {
  // 1. The deployment and the installer.
  const dep = lab.run('powershell', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'scripts/deploy.ps1', '-NoBuild']);
  const deployed = /Deployed AvLoop (\S+) .*: (\d+) files, ([\d.]+) MB/.exec(dep.out);
  w.check(dep.status === 0 && !!deployed, 'scripts\\deploy.ps1', hide(dep.out.trim().split(/\r?\n/).pop()));
  const version = deployed ? deployed[1] : '?';
  const shipped = fs.readdirSync(path.join(lab.REPO, 'deploy'));
  w.check(!shipped.some((f) => /^Qt6Network|^ffmpeg\.exe|^ffprobe/i.test(f)) && shipped.includes('vcruntime140.dll'),
    'deploy: the VC++ runtime beside, no network module, no FFmpeg tools', `${shipped.length} entries`);
  const built = lab.run(ISCC, ['/Q', 'installer/AvLoop.iss']);
  const setup = path.join(lab.REPO, 'installer', 'Output', `AvLoop-${version}-setup.exe`);
  w.check(built.status === 0 && fs.existsSync(setup), 'ISCC installer\\AvLoop.iss',
    fs.existsSync(setup) ? `${path.basename(setup)}, ${(fs.statSync(setup).size / 1048576).toFixed(1)} MB` : built.out.trim());

  // 2. A silent per-user install.
  const app = path.join(TMP, 'app');
  const inst = lab.run(setup, ['/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', `/DIR=${app}`]);
  const exe = path.join(app, 'avloop.exe');
  const licences = fs.existsSync(path.join(app, 'licenses')) ? fs.readdirSync(path.join(app, 'licenses')) : [];
  w.check(inst.status === 0 && fs.existsSync(exe) && fs.existsSync(path.join(app, 'unins000.exe')),
    'installed (per user, silent)', `exit ${inst.status}, ${fs.existsSync(app) ? fs.readdirSync(app).length : 0} entries`);
  w.check(licences.length === 4, 'the licences installed', licences.join(', '));
  const command = reg('HKCU\\Software\\Classes\\AvLoop.Project\\shell\\open\\command');
  w.check(reg('HKCU\\Software\\Classes\\.avloop') === 'AvLoop.Project' && command === `"${exe}" "%1"`,
    '.avloop → AvLoop.Project → "avloop.exe" "%1"', hide(command ?? 'no key'));
  w.check(/avloop\.exe",0$/.test(reg('HKCU\\Software\\Classes\\AvLoop.Project\\DefaultIcon') ?? ''), 'its icon: the app\'s',
    hide(reg('HKCU\\Software\\Classes\\AvLoop.Project\\DefaultIcon') ?? 'none'));
  w.check(fs.existsSync(startMenu), 'the Start menu entry', fs.existsSync(startMenu) ? path.basename(startMenu) : 'none');
  w.check(reg(uninstallKey, 'DisplayVersion') === version, 'Apps & features lists it with its version',
    `${reg(uninstallKey, 'DisplayName')} ${reg(uninstallKey, 'DisplayVersion')}`);

  // 3. The installed app on its own (only Windows on PATH, no Qt plugin path): a picture.
  const shot = lab.run(exe, ['--shot', path.join(TMP, 'installed.png')], { env });
  if (fs.existsSync(path.join(TMP, 'installed.png'))) fs.copyFileSync(path.join(TMP, 'installed.png'), path.join(HERE, 'installed.png'));
  w.check(shot.status === 0 && /PASS shot/.test(shot.out), 'the installed app starts and draws', hide(shot.out.trim()));

  // 4. The association's command with a project for %1.
  const project = writeProject(path.join(TMP, 'opened-by-double-click.avloop'),
    path.join(lab.REPO, 'samples', 'counter-640p30.mp4'), { crop: { kind: 'ellipse', x: 120, y: 40, w: 400, h: 280 } });
  const m = /^"([^"]+)" "%1"$/.exec(command ?? '');
  const script = path.join(TMP, 'assoc.json');
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 700, steps: [
    { do: 'waitFor', probe: 'video.state', eq: 'ready', timeout: 15000 },
    { do: 'expect', probe: 'window.title', contains: 'opened-by-double-click' },
    { do: 'expect', probe: 'crop.kind', eq: 'ellipse' },
    { do: 'shot', file: 'opened.png', maxWidth: 1200 },
  ] }));
  const opened = m ? lab.run(m[1], [project, '--walk', script, '--walk-out', TMP], { env }) : { status: -1, out: 'no command' };
  if (fs.existsSync(path.join(TMP, 'opened.png'))) fs.copyFileSync(path.join(TMP, 'opened.png'), path.join(HERE, 'opened.png'));
  w.check(opened.status === 0 && /^WALK PASSED/m.test(opened.out), 'the association opens the project',
    opened.out.split(/\r?\n/).filter((l) => /^(PASS|FAIL) expect/.test(l)).map(hide).join(' · '));

  // 5. Uninstalled: the uninstaller hands over to a copy of itself and returns; wait for the folder to go.
  lab.run(path.join(app, 'unins000.exe'), ['/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART']);
  let waited = 0;
  while (fs.existsSync(exe) && waited < 60000) { await sleep(500); waited += 500; }
  await sleep(1500);
  w.check(!fs.existsSync(app), 'the program folder gone', fs.existsSync(app) ? fs.readdirSync(app).join(', ') : `after ${waited} ms`);
  w.check(reg('HKCU\\Software\\Classes\\.avloop') === null && reg('HKCU\\Software\\Classes\\AvLoop.Project') === null,
    'the association gone', '.avloop and AvLoop.Project');
  w.check(!fs.existsSync(startMenu), 'the Start menu entry gone', fs.existsSync(startMenu) ? 'still there' : 'gone');
  w.check(reg(uninstallKey, 'DisplayName') === null, 'the uninstall entry gone', 'Apps & features');
  w.check(owned.every((d) => !fs.existsSync(d)), 'the autosave, logs and disk cache gone',
    owned.map((d) => `${hide(d)} ${fs.existsSync(d) ? 'left' : 'gone'}`).join(', '));
  w.check(fs.existsSync(marker), 'the settings folder kept (%APPDATA%\\AvLoop)', fs.existsSync(marker) ? 'kept' : 'gone');
} finally {
  // The owner's folders back as they were.
  owned.forEach((dir, i) => { if (aside[i]) fs.cpSync(aside[i], dir, { recursive: true }); });
  fs.rmSync(marker, { force: true });
  if (!settingsExisted) fs.rmSync(settings, { recursive: true, force: true });
  fs.rmSync(TMP, { recursive: true, force: true });
}
w.verdict();
