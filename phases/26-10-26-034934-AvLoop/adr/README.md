# ADRs of this programme

Two are owed. Each is drafted by the plan and **accepted by the phase that builds on it, before the code it
governs**; indexed in `DECISIONS.md` with its D-number and written into the lore (`docs/lore/avloop.md`) and
`CLAUDE.md`'s rules when its phase closes.

| ADR | Decision | Phase |
| --- | --- | --- |
| [AVADR-0001](AVADR-0001-one-renderer-gpu-cpu-parity.md) | Preview and export use one `FrameRenderer`; it has a GPU back end (OpenGL 3.3 core, offscreen) and a CPU back end of the same maths, one CPU-built mask shared by both, frames pre-scaled to the working size; parity held by `test-renderer` (≤ 2/255 unit cases; mean ≤ 1.0, max ≤ 4 projects). | AV-007 (accepted); AV-003 (the canvas's shader), AV-004 (the mask) build toward it |
| [AVADR-0002](AVADR-0002-memory-budgets-and-caches.md) | Every cache and queue has a fixed byte or count limit; frames are kept at the working resolution only; the RAM LRU spills to a per-session disk cache; lifetimes are tied to their owner and released on close; pressure halves the budgets. | AV-002 (accepted); AV-005, AV-007, AV-008, AV-009 apply it |
