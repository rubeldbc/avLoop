// AV-017's About walk (item 15, R-59, R-60): avloop.exe walked by a script made here — the window's title, the top
// bar, the ☰ menu's entry, the About box's labels and links read by the `about.text` probe, its pictures in Navy and
// Black. The contact details are read from src/ui/dialogs/AboutDialog.cpp, the one file that holds them (R-61): this
// walk's log names each by its row, never by its value, and the app's own log stays in the temporary folder.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const lab = await import(pathToFileURL(path.resolve(HERE, '..', '..', 'control', 'lab', 'av-lab.mjs')).href);
const w = lab.walkLog();
const NAME = 'AvLoop by Rubel';
const TMP = fs.mkdtempSync(path.join(os.tmpdir(), 'avloop-about-'));

const source = fs.readFileSync(path.join(lab.REPO, 'src', 'ui', 'dialogs', 'AboutDialog.cpp'), 'utf8');
const constant = (name) => new RegExp(`constexpr auto ${name} = "([^"]+)";`).exec(source)?.[1] ?? '';
const c = {
  developer: constant('kDeveloper'), email: constant('kEmail'), profile: constant('kProfile'),
  whatsApp: constant('kWhatsApp'), whatsAppLink: constant('kWhatsAppLink'), repository: constant('kRepository'),
};
w.check(Object.values(c).every(Boolean), 'the contact details, in AboutDialog.cpp', Object.keys(c).join(', '));
w.check(c.whatsAppLink === `https://wa.me/${c.whatsApp.replace(/\D/g, '')}`, "WhatsApp's link",
  'wa.me/ and the number shown, its digits only');
const link = (href, text) => `<a href="${href}">${text}</a>`;
// What the About box must show, by its row: a label's text, or a link with its target.
const rows = [
  ['the name', `<h2>${NAME}</h2>`],
  ['the version', '<p>Version 1.0.1</p>'],
  ['the repository, a link', link(`https://${c.repository}`, c.repository)],
  ['the developer', `|${c.developer}|`],
  ['the country', '|Bangladesh|'],
  ['the email, a mailto: link', link(`mailto:${c.email}`, c.email)],
  ['the developer profile, a link', link(`https://${c.profile}`, c.profile)],
  ['WhatsApp, a wa.me link', link(c.whatsAppLink, c.whatsApp)],
  ['the footer', `${c.developer} · Designed and built in Bangladesh`],
];
const absent = [['no glayer address', 'glayer'], ['no Website row', 'Website']];

const steps = [
  { do: 'expect', probe: 'window.title', eq: NAME },
  { do: 'shot', on: 'TopBar', file: 'topbar.png', maxWidth: 1400 },
  { do: 'menu', path: [`About ${NAME}`] },
  ...rows.map(([, text]) => ({ do: 'expect', probe: 'about.text', contains: text })),
  ...absent.map(([, text]) => ({ do: 'expect', probe: 'about.text', lacks: text })),
  { do: 'shot', on: 'AboutDialog', file: 'about-navy.png' },
  { do: 'click', on: 'about.close' },
  { do: 'wait', ms: 300 },
  { do: 'expect', probe: 'about.text', eq: '' },
  { do: 'theme', name: 'black' },
  { do: 'expect', probe: 'theme', eq: 'black' },
  { do: 'menu', path: [`About ${NAME}`] },
  { do: 'shot', on: 'AboutDialog', file: 'about-black.png' },
  { do: 'click', on: 'about.close' },
  { do: 'theme', name: 'navy' },
];
const script = path.join(TMP, 'about.json');
fs.writeFileSync(script, JSON.stringify({ delay: 120, settle: 700, steps }));

try {
  const ran = lab.app(['--walk', script, '--walk-out', TMP]);
  const said = ran.out.split(/\r?\n/).filter((l) => /^(PASS|FAIL) /.test(l));
  const about = said.filter((l) => / expect about\.text:/.test(l));
  const other = said.filter((l) => !/ expect about\.text:/.test(l));
  // The about.text lines come in the steps' order: the rows, what is absent, then the closed box.
  [...rows, ...absent, ['nothing once closed']].forEach(([what], i) =>
    w.check(about[i]?.startsWith('PASS') === true, `About: ${what}`, about[i] ? 'as written' : 'no reading'));
  for (const l of other) w.check(l.startsWith('PASS'), l.slice(5).split(':')[0], l.slice(l.indexOf(':') + 2));
  w.check(ran.status === 0 && /^WALK PASSED/m.test(ran.out), "the app's walk", `exit ${ran.status}`);
  for (const f of ['topbar.png', 'about-navy.png', 'about-black.png'])
    if (fs.existsSync(path.join(TMP, f))) fs.copyFileSync(path.join(TMP, f), path.join(HERE, f));
} finally {
  fs.rmSync(TMP, { recursive: true, force: true });
}
w.verdict();
