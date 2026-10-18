# Backlog

States: Todo / In progress / Review / Gate rerun (approved, start a new gate) / Gate (label) / Merged / Dropped (reason) / On hold (reason) / Manual (run by the human; the orchestrator leaves it alone).
Tracks: P performance, M memory, Q quality. Rotate P -> M -> Q.
Last track: Q

## Re-measure

The changes of the earlier runs were taken off wpe-vibes and are measured again on the current setup (headless
Weston, WPE_PLATFORM=wayland). Each is a branch on the base: task-r-{b1,b3,gpu1,gpu2,gpu3,gpu4,m3a,m3b};
task-r-m1 = gpu1..4 stacked, task-r-m3 = m3a + m3b. Run the gates from the main worktree after baseline.sh; they run
one at a time, so they can be queued together. Memory fixes share a gate: MotionMark only guards them against
regressions, their effect is the memory numbers.

| ID | Track | Target | Idea | State | Notes |
|---|---|---|---|---|---|
| R-AA | P | all | A/A gate: the base against itself (no commits, no target): false REJECT rate and bias between the main and gate build directories | Manual | `nohup setsid vibes/gate-run.sh g10-aa "" > /dev/null 2>&1 &`. Expected PASS. Record every test's change% and diff/se in KNOWLEDGE.md "A/A" (to decide the regression threshold of non-target tests). Nothing to merge |
| R-B1 | P | Images | getImageData: fast exact premul BGRA -> unpremul RGBA conversion instead of SkRasterPipeline (task-r-b1) | Manual | `nohup setsid vibes/gate-run.sh g11-b1 "$(git rev-list --reverse wpe-vibes..task-r-b1)" MotionMark/Images > /dev/null 2>&1 &` |
| R-B3 | P | CanvasLines, Paths | Lower per-draw dirty rect overhead of canvas 2D (task-r-b3) | Manual | `nohup setsid vibes/gate-run.sh g12-b3 "$(git rev-list --reverse wpe-vibes..task-r-b3)" MotionMark/CanvasLines MotionMark/Paths > /dev/null 2>&1 &` |
| R-M1 | M | all | GPU memory fixes 1-4 (task-r-m1): purge unused Skia resources after 5 s, MSAA only for canvas, purge the MSAA test surface, 256-multiple tile heights | Manual | `nohup setsid vibes/gate-run.sh g13-m1 "$(git rev-list --reverse wpe-vibes..task-r-m1)" > /dev/null 2>&1 &`. Memory: gpu-memory/run-compare.sh minibrowser, n=3, on base and task-r-m1 (per fix: base+gpu1, +gpu2, ... cumulative, to attribute) |
| R-M3 | M | process swap | Process swap fixes (task-r-m3): m3a destroys the renderer of a page suspended for a process swap; m3b does not cache a process that never committed a meaningful load | Manual | `nohup setsid vibes/gate-run.sh g14-m3 "$(git rev-list --reverse wpe-vibes..task-r-m3)" > /dev/null 2>&1 &`. Memory with the COOP repro on base, +m3a, +m3a+m3b: /tmp/m3/coop-server.py serves /coop with a `Cross-Origin-Opener-Policy: same-origin-allow-popups` header; run Tools/Scripts/webkit-gpu-memory against http://127.0.0.1:PORT/coop (WebDriver disables PSON, so only COOP responses swap processes; the about:blank page stays in the back/forward cache for 30 min). Plain PSON without automation: /tmp/m3/nav/run.sh. m3b changes a cross-port policy: drop it unless it shows an effect |
| R-ALL | - | - | Stack the passing R-* commits on task-r-all, gate it once with the P targets of the passers, then `vibes/gate.sh merge task-r-all <label>` | Manual | After the gates above. Then rebase task-m2 / task-q4 onto the new tip and set M2/Q4 to Gate rerun |

## Tasks

| ID | Track | Target | Idea | State | Notes |
|---|---|---|---|---|---|
| M2 | M | fractional scale | WPEPlatform (Wayland) has no wp_fractional_scale_v1: at 150% WPE renders at 2x. Implement fractional scale support (branch task-m2) | Todo (reviewer approved; after R-ALL rebase task-m2 onto the new tip and gate it) | Weston lacks the protocol (no-op there; the M gate should show no change). Test under headless sway (`WLR_BACKENDS=headless WLR_RENDER_DRM_DEVICE=/dev/dri/renderD128 WLR_RENDERER=gles2`, output scale 1.5, WPE_PLATFORM=wayland). Optional follow-ups: round the buffer size instead of truncating; the cursor at 1.5 uses the x1 theme size |
| Q4 | Q | css-color/system-color-support.html | RenderThemeAdwaita::systemColor: dark ButtonFace and Highlight ignore the dark color-scheme (branch task-q4) | Review (after R-ALL rebase task-q4 onto the new tip and gate it) | system-color-consistency.html is not fixable without a new WPE baseline (the generic expected file holds Mac FAIL lines): leave it [ Failure ] |
| M4 | M | reddit | Overlap layers never share a backing with siblings (46-58 overlap layers vs 5 Chrome layers) | Todo | Large and risky; analysis first |
| B6 | P | Images | AtlasUpload thread memmove (SkiaGPUAtlas::uploadImages) | Todo | Separate thread; check whether it is on the critical path |
| B7 | P | Leaves, Images | ImageQualityController::chooseInterpolationQuality, BitmapImageDescriptor::sourceSize per paint | Todo | Small |
| B9 | P | Suits | Batch per-frame CPU clip mask uploads (one texture per mask now) | Todo | Needs Skia changes |
| SP1 | P | Speedometer 3 / JetStream 2 | Add benchmarks to the P track (needs a gate script; orchestrator tooling) | On hold (tooling) | |
