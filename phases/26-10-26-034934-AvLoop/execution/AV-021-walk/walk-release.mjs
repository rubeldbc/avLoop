// AV-021's release walk (item 19, R-74, R-75): the version is the root CMakeLists.txt's 1.1.0, and every thing the
// Release carries says so — the built avloop-cli, scripts\deploy.ps1's deployment and its avloop-cli, avloop.exe's
// own details, ISCC's AvLoop-1.1.0-setup.exe and its details; the installer's SHA-256 written beside it, as the
// Release gives it. No silent install (R-75): the owner's own AvLoop, installed per user under the same AppId, keeps
// its Apps & features entry, its shortcuts and its association.
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const NAME = 'AvLoop by Rubel';
const AUTHOR = 'Md. Kamrul Islam Rubel';
const WANTED = '1.1.0';
const ISCC = [path.join(process.env['ProgramFiles(x86)'] ?? 'C:/Program Files (x86)', 'Inno Setup 6', 'ISCC.exe'),
  path.join(process.env.LOCALAPPDATA ?? '', 'Programs', 'Inno Setup 6', 'ISCC.exe')]
  .find((p) => fs.existsSync(p)) ?? 'ISCC.exe';
const appId = /AppId=\{\{([0-9A-F-]+)\}/.exec(fs.readFileSync(path.join(lab.REPO, 'installer', 'AvLoop.iss'), 'utf8'))[1];
const reg = (key, value) => {
  const r = lab.run('reg', ['query', key, '/v', value]);
  const m = /REG_\w+\s+(.*)$/m.exec(r.out);
  return r.status === 0 && m ? m[1].trim() : null;
};
const ownEntry = () => reg(`HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{${appId}}_is1`, 'DisplayVersion');
// A file's VERSIONINFO, as Explorer's Properties shows it (Inno's own pads its strings with spaces: trimmed).
const details = (file) => {
  const ps = `$v = (Get-Item -LiteralPath '${file}').VersionInfo; @{ product = $v.ProductName; ` +
    'company = $v.CompanyName; file = $v.FileVersion; version = $v.ProductVersion } | ConvertTo-Json -Compress';
  const r = lab.run('powershell', ['-NoProfile', '-Command', ps]);
  let read = {};
  try { read = JSON.parse(r.out); } catch { /* nothing read: every check below fails */ }
  return Object.fromEntries(Object.entries(read).map(([k, v]) => [k, String(v ?? '').trim()]));
};
const said = (d) => [d.product, d.company, d.file, d.version].join(' | ');
const firstLine = (r) => r.out.trim().split(/\r?\n/)[0] ?? '';

// 1. The one place the version is written.
const cmake = fs.readFileSync(path.join(lab.REPO, 'CMakeLists.txt'), 'utf8');
const VERSION = /project\(AvLoop VERSION (\S+)/.exec(cmake)?.[1];
w.check(VERSION === WANTED, 'the root CMakeLists.txt', `project(AvLoop VERSION ${VERSION})`);
const before = ownEntry();
w.note(before ? `the owner's own AvLoop installed here: ${before} (left as it is)` : 'no AvLoop installed here');

// 2. The build and the deployment.
const built = lab.cli(['version']);
w.check(built.status === 0 && firstLine(built) === `AvLoop ${VERSION}`, 'build\\bin\\avloop-cli version', firstLine(built));
const dep = lab.run('powershell', ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'scripts/deploy.ps1', '-NoBuild']);
const deployed = /Deployed AvLoop (\S+) in .*: (\d+) files, ([\d.]+) MB/.exec(dep.out);
w.check(dep.status === 0 && deployed?.[1] === VERSION, 'scripts\\deploy.ps1 -NoBuild',
  deployed ? `AvLoop ${deployed[1]}, ${deployed[2]} files, ${deployed[3]} MB` : dep.out.trim().split(/\r?\n/).pop());
const exe = details(path.join(lab.REPO, 'deploy', 'avloop.exe'));
w.check(exe.product === NAME && exe.company === AUTHOR && exe.version === VERSION && exe.file === VERSION,
  "deploy\\avloop.exe's details", said(exe));
const cli = lab.run(path.join(lab.REPO, 'deploy', 'avloop-cli.exe'), ['version']);
w.check(cli.status === 0 && firstLine(cli) === `AvLoop ${VERSION}`, 'deploy\\avloop-cli.exe version', firstLine(cli));

// 3. The installer, its details and its SHA-256.
const iscc = lab.run(ISCC, ['/Q', 'installer/AvLoop.iss']);
const setup = path.join(lab.REPO, 'installer', 'Output', `AvLoop-${VERSION}-setup.exe`);
const made = iscc.status === 0 && fs.existsSync(setup);
w.check(made, 'ISCC installer\\AvLoop.iss',
  made ? `${path.basename(setup)}, ${(fs.statSync(setup).size / 1048576).toFixed(1)} MB` : `${iscc.out}${iscc.err}`.trim());
const own = details(setup);
w.check(own.product === NAME && own.company === AUTHOR && own.version === VERSION, "the installer's details", said(own));
if (made) {
  const hash = crypto.createHash('sha256').update(fs.readFileSync(setup)).digest('hex');
  fs.writeFileSync(`${setup}.sha256`, `${hash}  ${path.basename(setup)}\n`);
  w.check(/^[0-9a-f]{64}$/.test(hash), 'its SHA-256, written beside it', `${hash} (${path.basename(setup)}.sha256)`);
}
w.check(ownEntry() === before, "the owner's own install untouched", `Apps & features: ${ownEntry() ?? 'none'}`);
w.verdict();
