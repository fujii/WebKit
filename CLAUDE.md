# WPE Vibes

This branch (`wpe-vibes`) is an experiment: AI agents alone improve the **quality** of the WPE port (performance,
GPU/CPU memory, failing/crashing/flaky tests). Nothing here goes upstream. Only WPE Release builds matter; GTK/Mac may
break. WPE mainly targets embedded devices, so the performance target is the Raspberry Pi 4; the work is done on a PC
(see `vibes/docs/rpi4-on-pc.md` for what transfers and what does not).

## Layout
All paths below are relative to the main worktree (the one with the branch `wpe-vibes` checked out). The scripts find
it themselves (`vibes/limits.sh`), so they can be run from any worktree.
- main worktree, branch `wpe-vibes`. Its build is the "before" side of every A/B measurement.
  Only `gate.sh merge` / `qgate.sh merge` put commits here. vibes/ state is left uncommitted during a run.
- `../wpe-vibes-w1`: the only worker worktree. Per task: `git switch -C task-<id> wpe-vibes`.
- `../wpe-vibes-gate`: gates (detached HEAD).
- `vibes/` (always the main worktree's copy):
  `BACKLOG.md` (tasks), `KNOWLEDGE.md` (profiles, findings), `LOG.md` (events; both created on first write), `ITERATION.md` (orchestrator prompt),
  `metrics/` (MotionMark JSON, `noise.json`, `lt-flaky.txt`, `usage.tsv`), `lt/`, `gates/`, `logs/`, `run/` (locks,
  pids, the tools pin and control files). Machine-specific settings go in `vibes/limits.local.sh` (see `vibes/limits.sh`).

## Tracks
- **P performance**: MotionMark 1.3.1 (later Speedometer 3 / JetStream 2). Gate: `gate.sh` (A/B statistics).
- **M memory**: GPU/CPU memory (findings in `gpu-memory/NOTES.md`, tool `Tools/Scripts/webkit-gpu-memory`,
  `gpu-memory/run-compare.sh`, run in a headless Weston). Gate: `gate.sh` with no target (MotionMark must not regress)
  plus memory numbers before/after in the task report, checked by the reviewer.
- **Q quality**: WPE layout tests marked Failure/Crash/Timeout in TestExpectations, crashes and flakiness seen on the
  bots (results.webkit.org, build.webkit.org), TestWebKitAPI and WebDriver failures. Only issues that reproduce on this
  branch's base. Gate: `qgate.sh`.

## Commands
- Every heavy job goes through `vibes/heavy.sh <name> <cmd...>` (global lock, memory checks, oom_score_adj 1000).
  Exit 75 = killed by memwatch: halve the parallelism and retry. Exit 76 = memwatch is not running (no live pid in
  `vibes/run/memwatch.pid`): restart it with `nohup setsid python3 vibes/memwatch.py >> vibes/logs/memwatch.log 2>&1 &`
  (from the main worktree).
- Priority: while `vibes/run/priority` exists, heavy.sh only starts jobs whose name matches the regex in it
  (gates go first). Other jobs wait; that is expected.
- Build: `vibes/heavy.sh build-w1 vibes/build.sh ../wpe-vibes-w1`
- MotionMark single test: `vibes/heavy.sh mm vibes/mm.sh <wt> <label> 1 MotionMark/Paths`. MotionMark runs in a
  headless Weston (`vibes/weston-vibes.sh`, 1920x1080, ~58.5 fps: the headless backend cannot do exactly 60) with
  `WPE_PLATFORM=wayland`. Compare only runs made with the same setup. `vibes/mm-score.py sanity <json>...` checks a
  run's environment (refresh rate, collapsed scores, the WPEPlatform module MiniBrowser loaded); baseline.sh and the
  gates refuse runs that fail it, and baseline.sh also fails when a test's CV exceeds `MM_MAX_NOISE_CV` (3%). Quick A/B: `vibes/heavy.sh mm-ab vibes/mm-ab.sh
  <A-wt> <B-wt> <label> 3 <Suite/Test>` then `vibes/mm-score.py compare --label <label> --target <Suite/Test>`.
- Layout tests: `vibes/heavy.sh lt vibes/lt-run.sh <wt> <name> <tests...>`; compare with
  `vibes/lt-diff.py vibes/lt/baseline-subset* -- vibes/lt/<name>`.
- API tests: `vibes/heavy.sh api bash -c 'cd <wt> && Tools/Scripts/run-wpe-tests --release <TestPath>'`.
- Gates (orchestrator only, detached): `nohup setsid vibes/gate-run.sh <label> "<commits>" <Suite/Test>... &`
  (P/M), `GATE_SCRIPT=qgate.sh nohup setsid vibes/gate-run.sh <label> "<commits>" lt|api|webdriver <args>... &` (Q).
  Labels: `g<N>-<task>`, each used once (the gate's job names must match `g[0-9]`). Gates wait for `vibes/baseline.sh`
  to finish and run one at a time: a second gate waits for the running one. `vibes/gates/<label>.txt` ends with the
  verdict: `PASS <commit>`, `REJECT: ...` or `BORDERLINE: ...` (MotionMark too close to call: run the gate again with
  a new label and `MM_ROUNDS_TARGET=10 MM_ROUNDS_ALL=6` before `nohup`). After reviewer approval:
  `vibes/gate.sh merge <branch> <label>` (or `qgate.sh merge`); it refuses when the branch changed after the gate or
  `wpe-vibes` moved since (then run the gate again with a new label).
- Profiling: `vibes/heavy.sh prof vibes/mm-profile.sh <wt> <label> <Suite/Test>` then `vibes/prof-report.sh <wt> <label>`.
  Read the report with `head -80`, never whole.

## Gates (all required)
- P: builds; MotionMark A/B (interleaved, target n=5, all tests n=3): target mean gain >= 2 standard errors of the
  difference and >= 1%; no other test drops > 1% and > 1.5 SE; layout test subset has no new failures; reviewer.
- M: same, but with no target (regressions only), and the reviewer checks the memory numbers.
- Q: builds; target tests pass every time (layout tests --repeat-each=5, API/WebDriver 3 runs); subset has no new
  failures; reviewer. API/WebDriver targets must be named (`TestName`, `TestName:/sub/test`, a WebDriver test file)
  and have no WPE expectation but PASS left: remove the expectation of the fixed test, else the runner counts a
  failure as expected.

## Forbidden (the gate or reviewer rejects)
- Editing the gate scripts and agent rules: `vibes/*.sh`, `vibes/*.py`, `vibes/*.ini`, `vibes/*.json`,
  `vibes/ITERATION.md`, `CLAUDE.md`, `.claude/`, `vibes/limits.local.sh` (partly denied in `.claude/settings.json`;
  the gates refuse to run when they differ from the commit pinned when the run started, `vibes/run/tools.pin`, even
  if the change is committed).
- Changing (the gates reject these paths): `Tools/Scripts/webkitpy/`, `Tools/Scripts/webkitperl/`, `webkitdirs.pm`,
  `build-webkit`, `Tools/Scripts/run-*`, `Tools/glib/`, `Tools/MiniBrowser/`, `PerformanceTests/`, `Source/cmake/`,
  the top-level `CMakeLists.txt`; existing tests: `LayoutTests/` files, `Tools/TestWebKitAPI/Tests/`, `WebDriverTests/`;
  a new `-expected` file of an existing test (a rebaseline). Adding new tests is fine. Exception: removing the
  expectation of a test the change fixes (LayoutTests/TestExpectations, LayoutTests/platform/{wpe,glib}/TestExpectations,
  Tools/TestWebKitAPI/glib/TestExpectations.json, WebDriverTests/TestExpectations.json); these files may only lose lines.
- Benchmark gaming: skipping/dropping frames, lowering quality (antialiasing, resolution, precision), changing
  timers/rAF/vsync/display link behaviour, detecting MotionMark, disabling sandbox or security checks, flipping feature
  flag or runtime setting defaults to switch rendering paths (only with explicit reviewer approval when the setting is
  on by default on Mac and WPE layout tests pass).
- Test gaming: making a test pass by special-casing it, weakening checks, or hiding a crash (e.g. an early return that
  skips the real work) instead of fixing the cause.
- LTO/PGO/-O3/-march/compiler changes; editing build configuration flags.
- `git push`, `git-webkit pr`, `git commit --no-verify`, posting to Bugzilla/GitHub/bots.
- Running heavy jobs outside `heavy.sh`; running two MotionMark runs at once.

## Token economy
- Never print whole logs, perf reports or results JSON. Use `tail`, `head -80`, `grep`, the scripts' one-line summaries.
- For bot/results lookups use the webkit-dev skills' scripts (`results.py`, `buildbot.py`) and read their summaries.
- Do not poll in a loop; a heavy job blocks until it finishes. Use Bash timeout up to 600000 ms and re-check.
- Reports from subagents: at most 30 lines.

## Conventions
- Comments and commit messages in English, WebKit ChangeLog style; the title describes the change.
- Prefer changes inside .cpp files; avoid widely included headers (unified build rebuild cost).
- Edit running scripts atomically (write a new file and `mv`), bash reads scripts while running them.
- Timebox a task to 90 minutes; at most 1 reviewer rework, then drop it and record why in `vibes/LOG.md`.
- Before implementing, check that the task is worth it: P needs >= 3% of profile samples in the target, Q needs a
  local reproduction on this branch. Otherwise drop it early.
- A change that legitimately needs rebaselines is put on hold (recorded in `vibes/LOG.md`), not merged.

## Starting a run
1. Create the worktrees: `git worktree add ../wpe-vibes-w1 -b task-init wpe-vibes`,
   `git worktree add --detach ../wpe-vibes-gate wpe-vibes`. Put machine-specific values in `vibes/limits.local.sh`.
2. `mkdir -p vibes/logs`, start memwatch (see Commands), build the main worktree
   (`vibes/heavy.sh build-main vibes/build.sh .`), then `nohup setsid vibes/baseline.sh > vibes/logs/baseline.log 2>&1 &`
   (it moves the results of an earlier baseline to `vibes/metrics/old-<timestamp>/`, and pins the gate scripts and
   agent rules: commit changes to them first).
3. `nohup setsid vibes/loop.sh <hours>|<HH:MM> > vibes/logs/loop.log 2>&1 &` (it pins them again, so a fix to the
   scripts committed between loops is trusted). Stop: `touch vibes/run/stop`.

## Resuming after an interruption
1. `tail -40 vibes/LOG.md`, `cat vibes/BACKLOG.md`, `git log --oneline origin/main..wpe-vibes`.
2. Check `ps -p "$(cat vibes/run/memwatch.pid)" -o args=` (restart memwatch if missing), `cat vibes/run/heavy.job`,
   `pgrep -af 'gate-run.sh|gate.sh|qgate.sh'` (a running gate).
3. Check the worker worktree: `git -C ../wpe-vibes-w1 status`.
4. Continue from the BACKLOG state (In progress / Gate / Review).
