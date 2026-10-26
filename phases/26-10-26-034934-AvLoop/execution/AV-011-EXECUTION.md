# AV-011 — Execution Record

**Phase:** AV-011 — The keyframes (owner item 11)
**Status:** DONE
**Date:** 2026-10-26
**Phase start commit:** `1d97d4b` (*[AV-015] CP-A1: track A walked whole on one fresh build*)
**ADR:** none owed (AVADR-0001: the one FrameRenderer takes the keyframes)
**Environment class:** local-disposable (this PC: `build/`, `samples/`, the walks' temp folders; lab windows behind the
owner's, never activated)

## The owner's words

*"Keyframes for video position and scale."*

## What was built

1. `src/core/Keyframes` — the keyframe, the value at a time (Linear, Ease = smoothstep, held before and after), one a
   frame (`withKey`, `moved` refused onto another), previous / next, the extremes for alpha and the working size.
2. `Project`: the transform at the playhead (`setTime`, `Aspect::Time`), `baseTransform()`, `transformAt()`;
   `addKeyframe`, `deleteKeyframe` (the last leaves the video where it was shown), `moveKeyframe` (one step a drag),
   `setKeyframeEase`; with keyframes `changeTransform` writes into the keyframe at the playhead (made if none), flips on
   the one transform; `SetKeyframesCommand` under the AddKeyframe / MoveKeyframe / DeleteKeyframe ids; the file's
   `transform.keyframes`; `exportFrame` and `outputAlpha` with keyframes.
3. `RenderJob` (`keyframes`, `transformAt`, `Placement::of(job, transform)`, alpha and working size over every
   keyframe) and `FrameRenderer` (each layer at its own source time).
4. UI: `ui/tools/KeyframeTool.cpp` (the options row; Delete), K adds/updates, `[` / `]`, the timeline's keyframe row
   (painting, click, drag, Delete, snapping), the playhead setting the project's time; panels, export and Preview
   ignore `Aspect::Time`; probes `keyframes.*` and the walk mapper `timelineKeys`.
5. `tests/test-keyframes.cpp` (7 cases).

## Decisions recorded

- **D-011** — the keyframes as delivered; spec §0.
- **R-47** flips hold; **R-48** the keyframe acted on is the one at the playhead; **R-49** no crop: the frame at In,
  Alpha Auto over every keyframe.

## Specification delta

- §0: `D-011`. Lore: *The keyframes* — four rules.

## Verification

| Command | Result |
| --- | --- |
| `scripts\build.ps1` | built, **0 warnings** (from clean objects after the header changes) |
| `ctest` | **21/21 PASS** (new: avloop-keyframes) |
| `scripts\format.ps1 -Check` | PASS (202 files) |
| `node tools/icons/check-icons.mjs` | PASS: 43 icons |
| `node tools/check-secrets.mjs` | PASS |
| `node control/phase-tools/check-loc.mjs` | PASS (the largest `src/ui/MainWindowProject.cpp`, 388 lines) |
| CP-A1's CI run | green, 7 m 45 s |

## The walk

```
node control/lab/walk-phase.mjs execution/AV-011-walk walk-keyframes.json walk-keyframes.mjs
walk-keyframes.json (8 s): the counter — K at 2 s: "2.000:320.0,180.0,1.000", "Keyframe 1 of 1 at 00:00:02.00";
  the Selection dragging the video +200 px at 4 s: a second keyframe, x 520; at 3 s x 420 (Linear, halfway) —
  picture; [ → 2 s; the tool's Interpolation Ease; at 2.5 s x 351.25 (smoothstep ¼ = 0.15625); ] → 4 s — options
  picture; the 4 s diamond dragged to 4.5 s, the playhead with it — row picture; Delete on the timeline → 1, Ctrl+Z → 2;
  the options' Delete → 1, the video at 320; [ and Delete again → none, "No keyframes: the video stays where it is",
  x 320; Ctrl+Z twice → 2; Add at a keyframe updates it (still 2)
walk-keyframes.mjs (5 s): a white 160×90 clip, keyframes x 200 (1 s) → 1000 (3 s), a 1280×720 crop, avloop-cli frame,
  the alpha's centroid — Linear ½ 600.00, Linear ¼ 400.00, Ease ¼ 325.00, Ease ½ 600.00, before 200.00, after
  1000.00 (y 360.00) — on the GPU and on the CPU alike
PHASE WALKS PASSED
```

## The pictures

| Picture | What | What it shows — and what was seen in it |
| --- | --- | --- |
| `AV-011-walk/keyframes-row.png` | the keyframe row | the rounded, hollow Ease diamond at 2 s; the solid diamond at 4.5 s under the playhead |
| `AV-011-walk/keyframe-options.png` | the options bar | Keyframe: Add, Delete, ◀ (enabled), ▶ (none after), Interpolation Linear, "Keyframe 2 of 2 at 00:00:04.00" |
| `AV-011-walk/keyframes-halfway.png` | the canvas at 3 s | the counter at 00:00:03.000 / 90 drawn 100 px right of where it starts — halfway |
| `AV-011-walk/linear-halfway.png` | avloop-cli frame 60 | the white clip centred at x 600 in the 1280×720 crop, transparent around |

## The regressions

| Walk of before | Result | Mended |
| --- | --- | --- |
| AV-001 … AV-010, every walk | PASSED | none |

Their pictures and logs were restored from git.

## External freshness

Not applicable — no dependency changed.

## Files and line counts

New: `src/core/Keyframes.{h,cpp}` (89), `src/ui/tools/KeyframeTool.cpp` (109), `tests/test-keyframes.cpp` (186).
Changed: `Project.*` (293), `Commands.h`, `ProjectJson.*`, `RenderJob.*`, `FrameRenderer.cpp`, `Timeline.*`,
`TimelinePaint.cpp`, `TimelineInput.cpp`, `MainWindow.cpp`, `MainWindowCanvas.cpp`, `MainWindowTimeline.cpp`,
`MainWindowExport.cpp`, `MainWindowPreview.cpp`, `CropPanel.cpp`, `LoopPanel.cpp`, `OutputPanel.cpp`, `Tool.h`, CMake.

## Recovery log

Attempt 1 RESOLVED — gate: the build — the SetTransform command needs its merge key.
Attempt 2 RESOLVED — gate: walk-keyframes.json (the diamond not grabbed, the playhead at 0) — the new walk mapper was
not built yet; rebuilt.
Attempt 3 RESOLVED — gate: walk-keyframes.json (no "options.keyframe.delete") — the options row prefixes names.
Attempt 4 RESOLVED — gate: walk-keyframes.json (Delete disabled at a keyframe) — a playhead move notifies with
keyframes even where the transform stays.

## Deviations from the phase contract

- **Code budget**: 450 planned; ~700 new lines + ~300 changed. The named seam (AV-011a: the row's drag and snapping)
  would move ~120. Recorded instead of split.
- **The README's pictures** were not retaken: the main window, the timeline and the crop look as they did (the row's
  diamonds appear only with keyframes); the keyframe row is pictured in the walk.

## Blocked legs (if any)

None.

## Staged paths

- `src/`, `tests/`, `README.md`, `docs/SPEC.md`, `docs/lore/avloop.md`, `phases/26-10-26-034934-AvLoop/`.

## Next

AV-012 — The Loop Finder, promoted to IN_PROGRESS in the same turn. No avloop process, session cache or walk temp file
left; the owner's focus untouched.
