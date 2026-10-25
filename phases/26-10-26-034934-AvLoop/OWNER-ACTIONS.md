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
| 6 | The GitHub Release `v1.0.0`. | At CP-B1, with the installer (R-5). | **Decided by the agent** — the README's Download link must work |
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
