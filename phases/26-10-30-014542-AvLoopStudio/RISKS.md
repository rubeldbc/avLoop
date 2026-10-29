# Risks

| # | Risk | What is done about it | Phase |
| --- | --- | --- | --- |
| 1 | **A broken app on main mid-way**: the model changes under every panel. | RS-7: the sequence lives beside AvLoop's model until each UI phase moves its part; every phase walks AvLoop's walks again. | AS-001 … AS-006 |
| 2 | **The CPU composite too slow** for the Preview at 1080p with several clips. | Rows in parallel (QtConcurrent), one pass a frame, the Preview's cache (R-31); the GL path for the monitor. | AS-002 |
| 3 | **Files at the 600-line gate** (Project, MainWindow, the timeline). | New files a concern each (`Sequence*`, `SequenceTimeline*`, `EffectControls*`); sizes read before work. | all |
| 4 | **A migrated AvLoop file renders differently.** | AS-002's walk renders AvLoop's walk projects before and after migration. | AS-001, AS-002 |
