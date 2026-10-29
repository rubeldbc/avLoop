# Owner actions

What only the owner would decide, with where the plan reads the owner's word. **Autonomous mode (spec §2.8): the
owner is asked nothing** — a row the owner's words do not close is "Decided by the agent" with its reason, built as
read and open for a veto later; an answer that differs changes the reading in its shape first.

| # | Action | Where the plan stands | Status |
| --- | --- | --- | --- |
| 1 | **The word to code** — and whether the checkpoints are stops. | AV-001 → AV-016 in order; the checkpoints are not stops. | **Given 2026-10-26** — the specification §2.8 ("The go-ahead is given") and "start" |
| 1b | **Commit the plan** (`[AV] Plan`) and push it to `origin` `main`. | Before AV-001's code. | **Given** — spec §2.8 item 1 |
| 2 | Commit per phase; push to `origin` `main`; never force. | Every close; retries 30 s, 2 min, 10 min. | **Standing** — spec §1.1 (later 5), §2.9 |
| 3 | Install the tools on this PC. | Qt by aqtinstall, `gh` and Inno Setup by winget, FFmpeg by `fetch-deps.ps1`. | **Given** — spec §2.8 item 5 |
| 4 | The credentials `gh` uses. | Git's credential manager's token passed per command (R-12). | **Decided by the agent** — §2.9 names the machine's credentials; `gh auth` was not logged in; nothing stored |
| 5 | Code signing. | None. | **Closed — the owner: "No code signing"** |
| 6 | The GitHub Release `v1.0.0`. | At CP-B1, with the installer (R-5). | **Taken at CP-B1 (AV-016)** — the installer and its SHA-256; the README's Download link works |
| 7 | Where the owner's original specification and mock live. | Left untracked where the owner put them; copies in `docs/` (R-11). | **Decided by the agent** — never move the owner's files |
| 8 | FFmpeg 7.1 from a dated release. | `n7.1.5` pinned, newest-7.1 fallback (R-9). | **Decided by the agent** — `latest` no longer carries 7.1 |
| 9 | Qt 6.8's patch release. | 6.8.3 (R-10). | **Decided by the agent** — the last open-source 6.8 LTS patch |
| 10 | The order of the project file's two halves; the encoder's home. | R-13, R-14. | **Decided by the agent** — the walks need projects early; Encoded preview precedes Export |
| 11 | **R-1** English UI, `tr()` everywhere. | `README.md` §3 1. | Decided by the agent (spec §13.2); open for a veto |
| 12 | **R-2** W/H unlocked never distorts. | `README.md` §3 2. | Decided by the agent (spec §13.2); open for a veto |
| 13 | **R-3** Space = Play/Pause. | `README.md` §3 3. | Decided by the agent (spec §13.2); open for a veto |
| 14 | **R-4** MIT licence. | `README.md` §3 4. | Decided by the agent (spec §13.2); open for a veto |
| 15 | **R-5** The Release at CP-B1. | `README.md` §3 5; row 6. | Decided by the agent (spec §13.2); open for a veto |
| 16 | **R-6** Default output. | `README.md` §3 6. | Decided by the agent (spec §13.2); open for a veto |
| 17 | **R-7** Premultiplied sRGB blending. | `README.md` §3 7. | Decided by the agent (spec §13.2); open for a veto |
| 18 | **R-8** One crop; none = the whole video. | `README.md` §3 8. | Decided by the agent (spec §13.2); open for a veto |
| 19 | **R-15** Animated WebP refused (no FFmpeg decodes it). | `README.md` §3 15; AV-002. | Decided by the agent (AV-002); open for a veto — a WebP decoder of AvLoop's own would be a new item |
| 20 | **R-16** The working size before a crop. | `README.md` §3 16; AV-002. | Decided by the agent (AV-002); open for a veto |
| 21 | **R-17 … R-20** QUndoStack in core; Reset at (0,0); Fit without a crop; the border with the Selection. | `README.md` §3 17 … 20; AV-003. | Decided by the agent (AV-003); open for a veto |
| 32 | **R-56 … R-58** Per-user install by default; what uninstalling keeps; no network module, the version from the exe. | `README.md` §3 56 … 58; AV-014. | Decided by the agent (AV-014); open for a veto |
| 31 | **R-51 … R-55** Pause restarts the entry; Ctrl+Shift+E's snapshot and free name; GIF's rules; MP4's bitrate; queue.json locked. | `README.md` §3 51 … 55; AV-013. | Decided by the agent (AV-013); open for a veto |
| 30 | **R-50** The Loop Finder's score and its ties. | `README.md` §3 50; AV-012. | Decided by the agent (AV-012); open for a veto |
| 29 | **R-47 … R-49** Flips hold; the keyframe at the playhead is the one acted on; no crop: the frame at In. | `README.md` §3 47 … 49; AV-011. | Decided by the agent (AV-011); open for a veto |
| 28 | **R-45, R-46** The Debug leak gate by heap growth; ASan judges errors, Release the growth. | `README.md` §3 45, 46; AV-015. | Decided by the agent (AV-015); open for a veto |
| 27 | **R-39 … R-44** The first format; the presets' long sides; the title of an unsaved video; the lab's answers; a changed source's toast; Settings saved at once. | `README.md` §3 39 … 44; AV-010. | Decided by the agent (AV-010); open for a veto |
| 26 | **R-34 … R-38** The Save dialog each time; the estimate after changes; `name (1).webm` without a question; Target size's MiB and correction; the output keys. | `README.md` §3 34 … 38; AV-009. | Decided by the agent (AV-009); open for a veto |
| 25 | **R-31 … R-33** The Preview's cache; VP8 in BT.601; one render thread and GPU context. | `README.md` §3 31 … 33; AV-008. | Decided by the agent (AV-008); open for a veto |
| 24 | **R-28 … R-30** Crossfade by default; a new rate moves In/Out to its grid; the Motion pre-pass runs for the Preview and the export. | `README.md` §3 28 … 30; AV-006. | Decided by the agent (AV-006); open for a veto |
| 23 | **R-24 … R-27** The range In up to Out (O keeps the frame); Dropped = never shown; timeline at 30 Hz while playing; the scrub's two speeds. | `README.md` §3 24 … 27; AV-005. | Decided by the agent (AV-005); open for a veto |
| 22 | **R-21 … R-23** Custom ratio from the crop; Feather seen in the Preview, not on the canvas; what snaps. | `README.md` §3 21 … 23; AV-004. | Decided by the agent (AV-004); open for a veto |
| 33 | **Item 15**: the name "AvLoop by Rubel" and the About box with the owner's contact — made public in the repository. | AV-017 (late). | **Given 2026-10-26** — the owner's words (`README.md` §1); told before the push that the phone number and the email become public, the owner's next word was "continue" |
| 34 | **R-59 … R-62** What keeps "AvLoop"; About's details; the gate's one address; 1.0.1 and its Release. | `README.md` §3 59 … 62; AV-017. | Decided by the agent (AV-017); open for a veto — the Release `v1.0.1` goes by `gh release delete v1.0.1` |
| 35 | **Item 16**: the canvas's wheel zooms about the pointer; Alt+drag copies a keyframe. | AV-018 (late). | **Given 2026-10-27** — the owner's words (`README.md` §1) |
| 36 | **R-63 … R-65** Ctrl+wheel kept, up/down panning by the wheel gone, the timeline's wheel unchanged; the copy one Undo step; no Release. | `README.md` §3 63 … 65; AV-018. | Decided by the agent (AV-018); open for a veto |
| 37 | **Item 17**: the Clip effects section — colour correction and looks before export. | AV-019 (late). | **Given 2026-10-27** — the owner's words (`README.md` §1) |
| 38 | **R-66 … R-70** Which effects; made on the CPU for the output, a close look on the canvas; the file's version 2; Opacity and Alpha; the section's place. | `README.md` §3 66 … 70; AV-019. | Decided by the agent (AV-019); open for a veto |
| 39 | **Item 18**: the colour theme — an orange clip made red, and the like. | AV-020 (late). | **Given 2026-10-27** — the owner's words (`README.md` §1) |
| 40 | **R-71 … R-73** The four theme effects and their order; the eyedropper; version 2 kept; the PC shut down after the push. | `README.md` §3 71 … 73; AV-020. | Decided by the agent (AV-020); the shutdown the owner's own word |
| 41 | **Item 19**: version 1.1.0 and its Release. | AV-021 (late). | **Given 2026-10-27** — the owner's words (`README.md` §1) |
| 42 | **R-74 … R-76** A minor step and the Release `v1.1.0`; no silent install over the owner's own AvLoop; the walks read the version. | `README.md` §3 74 … 76; AV-021. | Decided by the agent (AV-021); open for a veto — the Release goes by `gh release delete v1.1.0` |
| 43 | **Items 20 … 27**: the clip as cut; the inverse selection; the shapes and the pen; the shape combinations; the soft parts; the resets; the align tools and the shape layers; the timeline layers. | AV-022 … AV-031 (late). | **Given 2026-10-29** — the owner's words (`README.md` §1); the phase files first, then the owner's confirmation — **given 2026-10-29**: "commit, push and start and go till end of all phases" |
| 44 | **R-77 … R-103** The readings of items 20 … 27: Mode None; the inverse as a hole in the whole video; the file's version 3 once, unreleased through the set; shapes in the one frame, not keyframed; the four Pathfinder ops; Feather and Strength a shape; what a reset means a field; objects are the shapes; a clip's own shapes, no transitions or audio, at most 8 tracks. | `control/23` … `control/30`; `README.md` §3 as each phase closes. | Decided by the agent (planning, 2026-10-29); open for a veto — the owner reads them before the confirmation |
