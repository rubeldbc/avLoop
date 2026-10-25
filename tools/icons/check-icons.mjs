// The icons' gate (spec §5.3, §14.1 Form): every SVG of resources/icons has the 24×24 viewBox, stroke-width 1.75,
// stroke="currentColor", fill="none", round caps and joins, and no colour of its own; every icon IconFactory lists
// exists, is in resources.qrc, and no SVG stands unlisted; the app icon (resources/app/avloop.ico) holds seven sizes.
// Node built-ins only. Exit 0 = PASS, 1 = FAIL.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const ICONS = path.join(ROOT, 'resources', 'icons');
const errors = [];
const fail = (m) => errors.push(m);

const factory = fs.readFileSync(path.join(ROOT, 'src', 'ui', 'theme', 'IconFactory.cpp'), 'utf8');
const block = factory.slice(factory.indexOf('// icons:begin'), factory.indexOf('// icons:end'));
const listed = [...block.matchAll(/QStringLiteral\("([a-z0-9-]+)"\)/g)].map((m) => m[1]);
if (listed.length === 0) fail('IconFactory.cpp: no icon list between icons:begin and icons:end');

const qrc = fs.readFileSync(path.join(ROOT, 'resources', 'resources.qrc'), 'utf8');
const files = fs.readdirSync(ICONS).filter((f) => f.endsWith('.svg'));
const colour = /#[0-9a-f]{3,8}\b|\brgba?\(|\bhsla?\(|(?:fill|stroke|stop-color|color)\s*=\s*"(?!currentColor|none)[^"]*"/i;

for (const f of files) {
  const name = f.replace(/\.svg$/, '');
  const svg = fs.readFileSync(path.join(ICONS, f), 'utf8');
  const root = svg.match(/<svg\b[^>]*>/)?.[0] ?? '';
  const want = {
    viewBox: '0 0 24 24', 'stroke-width': '1.75', stroke: 'currentColor', fill: 'none',
    'stroke-linecap': 'round', 'stroke-linejoin': 'round',
  };
  for (const [attr, value] of Object.entries(want)) {
    if (!root.includes(`${attr}="${value}"`)) fail(`${f}: the root lacks ${attr}="${value}"`);
  }
  const body = svg.replace(root, '');
  if (colour.test(body)) fail(`${f}: a colour of its own (${body.match(colour)[0]})`);
  if (!listed.includes(name)) fail(`${f}: not listed in IconFactory::allNames()`);
  if (!qrc.includes(`<file>icons/${f}</file>`)) fail(`${f}: not in resources/resources.qrc`);
}
for (const name of listed) if (!files.includes(`${name}.svg`)) fail(`${name}: listed but resources/icons/${name}.svg is missing`);

const ico = path.join(ROOT, 'resources', 'app', 'avloop.ico');
if (!fs.existsSync(ico)) fail('resources/app/avloop.ico is missing (build avloop-iconbake and run it on avloop.svg)');
else {
  const b = fs.readFileSync(ico);
  const count = b.readUInt16LE(4);
  const sizes = [];
  for (let i = 0; i < count; i++) sizes.push(b[6 + i * 16] || 256);
  const wanted = [16, 24, 32, 48, 64, 128, 256];
  if (wanted.some((s) => !sizes.includes(s))) fail(`avloop.ico has sizes ${sizes.join(', ')}; wanted ${wanted.join(', ')}`);
}

for (const e of errors) console.log(`FAIL ${e}`);
console.log(errors.length ? `FAIL: ${errors.length} problem(s) in ${files.length} icons` : `PASS: ${files.length} icons, ${listed.length} listed, avloop.ico with 7 sizes`);
process.exit(errors.length ? 1 : 0);
