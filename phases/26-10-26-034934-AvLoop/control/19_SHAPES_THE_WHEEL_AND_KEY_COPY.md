# 19 Shapes — the wheel and the keyframe copy (item 16, AV-018, late)

Canonical current execution state: `../AUTONOMOUS_STATE.md`

The owner's words of 2026-10-27, after AV-017 closed (`../README.md` §1, item 16). D-016. Readings R-63 … R-65.

## 1. The canvas's wheel (`src/ui/canvas/Canvas.cpp`)

- The wheel, with Ctrl or without, zooms about the pointer: the scene point under it stays under it. One notch
  (`QWheelEvent::DefaultDeltasPerStep`, 120 eighths of a degree) is one step of §6.3's list (10 … 800 %); a finer
  wheel or a touchpad adds its deltas up to a notch, and a turn the other way starts the count again.
- Shift+wheel (which Windows may turn sideways) and a sideways wheel pan across, half a view pixel an eighth of a
  degree, as before. Panning up and down by the wheel is gone: a middle-drag or the Hand pan every way (R-63).
- The view is never in Undo. The timeline's wheel is unchanged (the owner named the canvas).

## 2. The keyframe copy (`src/ui/timeline/TimelineInput.cpp`)

- Alt pressed on a diamond: the drag's first step off its frame copies the keyframe there — position, scale and
  interpolation — and the original stays; the copy then follows the pointer on the output grid. Refused where
  another keyframe already is (one a frame); the copy stays at its last place.
- One Undo step, "Copy a keyframe": `Project::copyKeyframe` pushes with MoveKeyframe's id and the drag's merge key, so
  the copy's moves merge into it. Alt+click without a drag copies nothing; the playhead goes to the diamond, as a
  click does.
- The copy cursor shows with Alt over a diamond.
- Mend (R-64): a drag refused over another keyframe keeps the one it drags — before, the drag picked the other one
  up and moved it on.

## 3. The version

No new version and no Release (R-65): the next Release carries it.

## 4. Walk

`walk-wheel.json`: Fit then 100 %; the wheel at scene (100, 50) → 150 % and 200 %, the pan such that the point stays
under the pointer; half a notch nothing, the second half a step out; Ctrl+wheel as before back to 100 % and the view
where it began; Shift+wheel and a sideways wheel pan across; no Undo step. Two pictures. `walk-key-copy.json`: K at
2 s, X typed at 4 s; Alt+drag 2 s → 3 s: three keyframes, the copy's values the original's, one Undo step, Undo and
Redo; Alt+click copies nothing; a plain drag 3 s → 4.5 s across 4 s keeps its own. Two pictures.
