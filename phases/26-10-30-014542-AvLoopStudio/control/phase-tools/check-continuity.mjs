// The owner's mechanical test for the standing order in CONTINUE.md.
// Exit 0: WORKING (a phase is active), FINISHED (nothing left), PLANNED (never started).
// Exit 1: STOPPED (an eligible phase is parked while nothing is active).
import { readProgress, ACTIVE_STATUSES } from './lib.mjs';

const progress = readProgress();
if (!progress) { console.log('STOPPED: PROGRESS.json missing'); process.exit(1); }

const phases = progress.phases ?? [];
const active = phases.find((p) => ACTIVE_STATUSES.includes(p.status));
if (active) { console.log(`WORKING: ${active.id} is ${active.status}`); process.exit(0); }

if (progress.state === 'PLANNED') { console.log(`PLANNED: not launched yet; first phase ${progress.nextEligiblePhase}`); process.exit(0); }
if (progress.state === 'PAUSED') { console.log('PAUSED: the owner paused the programme'); process.exit(0); }

if (progress.nextEligiblePhase) {
  console.log(`STOPPED: ${progress.nextEligiblePhase} is eligible and nothing is active — promote it and continue`);
  process.exit(1);
}
console.log(`FINISHED: ${progress.completed}/${progress.total} done; blocked ${progress.blocked.length}; deferred ${progress.deferred.length}`);
process.exit(0);
