// The public repository's gate (spec §2.9): no token, key, password, email address or personal path in any file git
// would commit (tracked, or untracked and not ignored). Runs before every commit. Node built-ins only.
// Exit 0 = PASS, 1 = FAIL.
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const BINARY = /\.(png|ico|jpg|jpeg|gif|webm|mp4|mov|mkv|zip|exe|dll|lib|pdb|obj)$/i;
const RULES = [
  ['GitHub token', /\bgh[pousr]_[A-Za-z0-9]{30,}\b|\bgithub_pat_[A-Za-z0-9_]{20,}/],
  ['AWS key', /\bAKIA[0-9A-Z]{16}\b/],
  ['private key', /-----BEGIN (?:RSA |EC |DSA |OPENSSH |PGP )?PRIVATE KEY-----/],
  ['Slack token', /\bxox[abpr]-[A-Za-z0-9-]{10,}/],
  ['secret assignment', /\b(?:api[_-]?key|secret|passw(?:or)?d|access[_-]?token)\b\s*[:=]\s*['"][^'"\s]{12,}['"]/i],
  ['email address', /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)*\.[A-Za-z]{2,}\b/],
  ['personal path', /\b[A-Za-z]:[\\/]+Users[\\/]+(?!Public\b|Default\b|runneradmin\b|<|%|\$|\{)[^\\/\s"'`<>|]+/i],
  ['personal path', /\/(?:[a-z]\/)?Users\/(?!Public\b|Default\b|<|%|\$|\{)[A-Za-z][^/\s"'`]*/],
  ['personal path', /\/home\/(?!runner\b|<|\$|\{)[a-z][a-z0-9_-]*\//],
];
// Addresses that are no one's: placeholders and the hosting service's no-reply domains.
const ALLOWED_EMAIL = /@(?:example\.(?:com|org)|users\.noreply\.github\.com|noreply\.github\.com)$/i;

const listed = execFileSync('git', ['ls-files', '-z', '--cached', '--others', '--exclude-standard'], { cwd: ROOT })
  .toString('utf8').split('\0').filter(Boolean);
const files = listed.filter((f) => !BINARY.test(f) && fs.existsSync(path.join(ROOT, f)));

const findings = [];
for (const f of files) {
  const text = fs.readFileSync(path.join(ROOT, f), 'utf8');
  const lines = text.split(/\r?\n/);
  lines.forEach((line, i) => {
    for (const [kind, re] of RULES) {
      const m = line.match(re);
      if (!m) continue;
      if (kind === 'email address' && ALLOWED_EMAIL.test(m[0])) continue;
      if (kind === 'email address' && /^[^@]+@\d+x\./.test(m[0])) continue; // image@2x.png
      findings.push(`${f}:${i + 1}: ${kind} (${m[0].slice(0, 12)}…)`);
    }
  });
}
for (const x of findings) console.log(`FAIL ${x}`);
console.log(findings.length ? `FAIL: ${findings.length} finding(s) in ${files.length} files` : `PASS: ${files.length} files, nothing secret or personal`);
process.exit(findings.length ? 1 : 0);
