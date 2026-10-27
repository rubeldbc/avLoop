// AV-017's name walk (item 15, R-59, R-62): scripts\deploy.ps1 and ISCC make the installer; avloop.exe's and the
// installer's own details read; a stand-in of 1.0.0's "AvLoop.lnk" in the Start menu; a silent per-user install into a
// temporary folder replaces it by "AvLoop by Rubel.lnk"; Apps & features names the app, its publisher and its
// version; the installed app's title (a picture); the silent uninstall leaves no shortcut and no entry. The owner's
// AvLoop folders (which uninstalling clears) are kept aside and put back.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const NAME = 'AvLoop by Rubel';
const AUTHOR = 'Md. Kamrul Islam Rubel';
// The version as the root CMakeLists.txt gives it (1.0.1 at AV-017, 1.1.0 from AV-021).
const VERSION = /project\(AvLoop VERSION (\S+)/.exec(fs.readFileSync(path.join(lab.REPO, 'CMakeLists.txt'), 'utf8'))?.[1];
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-name-'));
const hide = (t) => t.split(TMP).join('…').split(os.homedir()).join('~');
const ISCC = [path.join(process.env['ProgramFiles(x86)'] ?? 'C:/Program Files (x86)', 'Inno Setup 6', 'ISCC.exe'),
  path.join(process.env.LOCALAPPDATA ?? '', 'Programs', 'Inno Setup 6', 'ISCC.exe')]
  .find((p) => fs.existsSync(p)) ?? 'ISCC.exe';
const LOCAL = process.env.LOCALAPPDATA;
const owned = [path.join(LOCAL, 'AvLoop'), path.join(LOCAL, 'cache', 'AvLoop')]; // what uninstalling removes
const programs = path.join(process.env.APPDATA, 'Microsoft', 'Windows', 'Start Menu', 'Programs');
const oldLink = path.join(programs, 'AvLoop.lnk');
const newLink = path.join(programs, `${NAME}.lnk`);
const appId = /AppId=\{\{([0-9A-F-]+)\}/.exec(fs.readFileSync(path.join(lab.REPO, 'installer', 'AvLoop.iss'), 'utf8'))[1];
const uninstallKey = `HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{${appId}}_is1`;
const reg = (key, value) => {
  const r = lab.run('reg', ['query', key, '/v', value]);
  const m = /REG_\w+\s+(.*)$/m.exec(r.out);
  return r.status === 0 && m ? m[1].trim() : null;
};
// A file's VERSIONINFO, as Explorer's Properties shows it (Inno's own pads its strings with spaces: trimmed).
const details = (file) => {
  const ps = `$v = (Get-Item -LiteralPath '${file}').VersionInfo; @{ product = $v.ProductName; ` +
    'company = $v.CompanyName; description = $v.FileDescription; copyright = $v.LegalCopyright; ' +
    'version = $v.ProductVersion } | ConvertTo-Json -Compress';
  const r = lab.run('powershell', ['-NoProfile', '-Command', ps]);
  let read = {};
  try { read = JSON.parse(r.out); } catch { /* nothing read: every check below fails */ }
  return Object.fromEntries(Object.entries(read).map(([k, v]) => [k, String(v ?? '').trim()]));
};
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const windowsOnly = `${process.env.SystemRoot}\\System32;${process.env.SystemRoot}`;
const env = { PATH: windowsOnly, Path: windowsOnly, QT_PLUGIN_PATH: '' };

const aside = owned.map((dir, i) => {
  if (!fs.existsSync(dir)) return null;
  const copy = path.join(TMP, `aside-${i}`);
  fs.cpSync(dir, copy, { recursive: true });
  return copy;
});
const oldLinkAside = fs.existsSync(oldLink) ? fs.readFileSync(oldLink) : null;

try {
  // 1. The deployment, the installer and their details.
  const dep = lab.run('powershell', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'scripts/deploy.ps1', '-NoBuild']);
  const deployed = /Deployed AvLoop (\S+) /.exec(dep.out);
  w.check(dep.status === 0 && deployed?.[1] === VERSION, 'scripts\\deploy.ps1', hide(dep.out.trim().split(/\r?\n/).pop()));
  const exe = details(path.join(lab.REPO, 'deploy', 'avloop.exe'));
  const said = (d) => [d.product, d.company, d.description, d.copyright, d.version].join(' | ');
  w.check(exe.product === NAME && exe.company === AUTHOR && exe.version === VERSION
    && exe.description?.startsWith(`${NAME} - `) && exe.copyright?.includes(AUTHOR), "avloop.exe's details", said(exe));
  const built = lab.run(ISCC, ['/Q', 'installer/AvLoop.iss']);
  const setup = path.join(lab.REPO, 'installer', 'Output', `AvLoop-${VERSION}-setup.exe`);
  w.check(built.status === 0 && fs.existsSync(setup), 'ISCC installer\\AvLoop.iss',
    fs.existsSync(setup) ? `${path.basename(setup)}, ${(fs.statSync(setup).size / 1048576).toFixed(1)} MB` : built.out.trim());
  const own = details(setup);
  w.check(own.product === NAME && own.company === AUTHOR && own.copyright?.includes(AUTHOR), "the installer's details",
    said(own));

  // 2. 1.0.0's shortcut, stood in; a silent per-user install over it.
  fs.mkdirSync(programs, { recursive: true });
  fs.writeFileSync(oldLink, 'a stand-in of 1.0.0\'s shortcut (AV-017 walk)\n');
  const app = path.join(TMP, 'app');
  const inst = lab.run(setup, ['/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', `/DIR=${app}`]);
  w.check(inst.status === 0 && fs.existsSync(path.join(app, 'avloop.exe')), 'installed (per user, silent)', `exit ${inst.status}`);
  w.check(fs.existsSync(newLink), 'the Start menu entry', fs.existsSync(newLink) ? path.basename(newLink) : 'none');
  w.check(!fs.existsSync(oldLink), "1.0.0's \"AvLoop.lnk\" removed", fs.existsSync(oldLink) ? 'still there' : 'gone');
  const shown = [reg(uninstallKey, 'DisplayName'), reg(uninstallKey, 'Publisher'), reg(uninstallKey, 'DisplayVersion')];
  w.check(shown[0] === NAME && shown[1] === AUTHOR && shown[2] === VERSION, 'Apps & features', shown.join(' | '));
  const assoc = lab.run('reg', ['query', 'HKCU\\Software\\Classes\\.avloop', '/ve']).out;
  w.check(/REG_SZ\s+AvLoop\.Project/.test(assoc), 'the association keeps its name', '.avloop → AvLoop.Project');

  // 3. The installed app (only Windows on PATH): its title.
  const script = path.join(TMP, 'title.json');
  fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 700, steps: [
    { do: 'expect', probe: 'window.title', eq: NAME },
    { do: 'shot', file: 'installed.png', maxWidth: 1200 },
  ] }));
  const ran = lab.run(path.join(app, 'avloop.exe'), ['--walk', script, '--walk-out', TMP], { env });
  const picture = path.join(TMP, 'installed.png');
  if (fs.existsSync(picture)) fs.copyFileSync(picture, path.join(HERE, 'installed.png'));
  w.check(ran.status === 0 && /^WALK PASSED/m.test(ran.out), 'the installed app is titled by the name',
    ran.out.split(/\r?\n/).filter((l) => /^(PASS|FAIL) /.test(l)).map(hide).join(' · '));

  // 4. Uninstalled (the uninstaller hands over to a copy of itself): no shortcut, no entry.
  lab.run(path.join(app, 'unins000.exe'), ['/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART']);
  let waited = 0;
  while (fs.existsSync(path.join(app, 'avloop.exe')) && waited < 60000) { await sleep(500); waited += 500; }
  await sleep(1500);
  w.check(!fs.existsSync(app), 'the program folder gone', `after ${waited} ms`);
  w.check(!fs.existsSync(newLink) && !fs.existsSync(oldLink), 'no shortcut left', 'neither name');
  w.check(reg(uninstallKey, 'DisplayName') === null, 'the uninstall entry gone', 'Apps & features');
} finally {
  owned.forEach((dir, i) => { if (aside[i]) fs.cpSync(aside[i], dir, { recursive: true }); });
  if (oldLinkAside) fs.writeFileSync(oldLink, oldLinkAside);
  else fs.rmSync(oldLink, { force: true });
  fs.rmSync(TMP, { recursive: true, force: true });
}
w.verdict();
