// The owner's hard rule (spec §2.4, CLAUDE.md, EXECUTOR-PREAMBLE §1): no code file over 600 lines, anywhere in the
// repository. Exit 0 = PASS, 1 = FAIL.
// "Anywhere": the whole working tree is walked from the repository root, for the kinds the spec names — .c, .cpp, .h,
// .hpp, .cmake, CMakeLists.txt, .qss, .mjs, .js, .ps1, .py, .iss, .glsl, .frag, .vert, .qrc, .ui. A kind of code file
// not named here is added in the phase that brings it.
// Never scanned: git's own folder, fetched dependencies (third_party/), builds (build*/, out/), a package install
// (node_modules/), the installer's output (installer/Output/). Programme documents and tools under phases/ have a
// stricter ceiling of their own, 300 lines (validate-phase-system.mjs); they are counted here too.
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, countLines } from './lib.mjs';

const LIMIT = 600;
const EXTENSIONS = new Set([
  '.c', '.cpp', '.h', '.hpp', '.cmake', '.qss', '.mjs', '.js', '.ps1', '.py', '.iss', '.glsl', '.frag', '.vert', '.qrc',
  '.ui',
]);
const SKIP_NAMES = new Set(['.git', 'third_party', 'node_modules', 'out', '.vs', '.cache']);
const SKIP_RELS = new Set([path.join('installer', 'Output')]);

const isCode = (file) => EXTENSIONS.has(path.extname(file).toLowerCase()) || path.basename(file) === 'CMakeLists.txt';
const skipped = (name, rel) => SKIP_NAMES.has(name) || SKIP_RELS.has(rel) || /^build/i.test(name);

function walk(dir, out) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    const rel = path.relative(REPO_ROOT, full);
    if (entry.isDirectory()) {
      if (!skipped(entry.name, rel)) walk(full, out);
    } else if (isCode(full)) {
      out.push(full);
    }
  }
  return out;
}

const files = walk(REPO_ROOT, []);
const counted = files.map((file) => ({ file: path.relative(REPO_ROOT, file).replace(/\\/g, '/'), lines: countLines(file) }));
counted.sort((a, b) => b.lines - a.lines);
const over = counted.filter((c) => c.lines > LIMIT);

for (const c of over) console.log(`FAIL ${c.file}: ${c.lines} lines exceeds the ${LIMIT}-line limit`);
if (over.length) {
  console.log(`FAIL: ${over.length} of ${counted.length} code files exceed ${LIMIT} lines`);
  process.exit(1);
}
const largest = counted[0];
console.log(`PASS: ${counted.length} code files, none over ${LIMIT} lines (the largest: ${largest ? `${largest.file}, ${largest.lines} lines` : 'none'})`);
