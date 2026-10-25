// Watches the CI run of a pushed commit (EXECUTOR-PREAMBLE §4 step 9; reading R-12): the GitHub token comes from git's
// credential manager for this one process tree as GH_TOKEN — never printed, never stored. Prints the run's jobs and
// steps and exits with the run's verdict (0 green). Node built-ins only.
//   node ci-watch.mjs [<commit sha>]   (default: HEAD)
import { execFileSync, spawnSync } from 'node:child_process';

const GH = 'C:\\Program Files\\GitHub CLI\\gh.exe';
const REPO = 'rubeldbc/avLoop';
const sha = process.argv[2] ?? execFileSync('git', ['rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
const cred = spawnSync('git', ['credential', 'fill'], { input: 'protocol=https\nhost=github.com\n\n', encoding: 'utf8', env: { ...process.env, GIT_TERMINAL_PROMPT: '0' } });
const token = cred.stdout.match(/^password=(.*)$/m)?.[1];
if (!token) { console.log('NO TOKEN: git has no GitHub credential'); process.exit(2); }
const env = { ...process.env, GH_TOKEN: token };
const gh = (args, opts = {}) => spawnSync(GH, args, { encoding: 'utf8', env, ...opts });

let id = null;
for (let i = 0; i < 40 && !id; i++) {
  const r = gh(['run', 'list', '--repo', REPO, '--commit', sha, '--limit', '1', '--json', 'databaseId', '--jq', '.[0].databaseId']);
  id = r.stdout.trim() || null;
  if (!id) spawnSync(process.execPath, ['-e', 'setTimeout(()=>{},15000)']);
}
if (!id) { console.log(`NO RUN for ${sha.slice(0, 7)} after 10 minutes`); process.exit(3); }
console.log(`run ${id} for ${sha.slice(0, 7)}`);
const watch = gh(['run', 'watch', id, '--repo', REPO, '--exit-status', '--interval', '30'], { timeout: 3600000 });
const lines = `${watch.stdout}`.split(/\r?\n/);
const tail = lines.slice(lines.findIndex((l) => /^(✓|X) /.test(l)));
console.log(tail.join('\n').trim());
const view = gh(['run', 'view', id, '--repo', REPO, '--json', 'conclusion,updatedAt,createdAt', '--jq', '"\\(.conclusion) \\(.createdAt) \\(.updatedAt)"']);
console.log(`conclusion: ${view.stdout.trim()}`);
if (watch.status !== 0) {
  const log = gh(['run', 'view', id, '--repo', REPO, '--log-failed']);
  console.log(`${log.stdout}`.split(/\r?\n/).slice(-60).join('\n'));
}
process.exit(watch.status === 0 ? 0 : 1);
