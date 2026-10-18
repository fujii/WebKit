# WPE Vibes

An experiment in which AI agents (Claude Code) alone improve the quality of the WPE WebKit port: rendering
performance (MotionMark), GPU/CPU memory, and failing/crashing/flaky tests. The branch is not meant for upstream; the
changes on it are candidates that a human still has to review before proposing any of them.

WPE mainly targets embedded devices, so the performance target is the Raspberry Pi 4. Measuring on the device is slow,
so the agents work on a PC; [docs/rpi4-on-pc.md](docs/rpi4-on-pc.md) discusses what PC measurements tell about the
Raspberry Pi 4.

## How it works
- `loop.sh` starts one non-interactive `claude -p` orchestrator session per iteration with the prompt `ITERATION.md`.
  The orchestrator picks a task from `BACKLOG.md`, runs the `vibes-worker` agent on it in a worker worktree, has the
  `vibes-reviewer` agent review the result, and runs a gate.
- Gates (`gate.sh`, `qgate.sh`) decide merges from numbers: interleaved MotionMark A/B with noise-aware thresholds,
  a layout test subset with no new failures, target tests passing every time. The agents may not edit the gate
  scripts or their own rules (`.claude/settings.json` denies it, the gates check the diff paths, and they refuse to run
  copies that differ from the ones pinned when the run started). The scripts shared by both gates are in `gate-lib.sh`.
- `heavy.sh` serializes heavy jobs (builds, tests, benchmarks) under one lock, and `memwatch.py` kills a job before
  the machine runs out of memory.
- The rules the agents follow are in `../CLAUDE.md`; findings accumulate in `KNOWLEDGE.md`, events in `LOG.md`.

## Requirements
- A Linux PC that can build WPE (e.g. the [WebKit container SDK](https://github.com/Igalia/webkit-container-sdk)),
  with a Mesa GPU driver, Weston (headless backend) and `perf`.
- Claude Code. The agents use the webkit-dev skills (results.webkit.org, build.webkit.org helpers) when a plugin
  providing them is set with `VIBES_PLUGIN_DIR`; without it the Q track triage is weaker.
- Machine-specific values (parallelism, memory thresholds, CPU pinning) are derived from the machine; override them in
  `limits.local.sh`.

**Safety**: `loop.sh` runs Claude Code with `--permission-mode auto` and no permission prompts for hours. Run it only
on a machine or container you are willing to let an agent use, without credentials it should not have. `git push`
and posting to WebKit services are denied in `.claude/settings.json`, but that is not a sandbox.

## Running
See "Starting a run" in `../CLAUDE.md`.

## Results so far
Runs 1-3 (2026-10-16 to 10-18) ran on an older base. Their MotionMark numbers were measured with the
wpe-platform-gtk module by accident and are not comparable with later runs, so the changes merged then
were taken off this branch to be measured and gated again (see "Re-measure" in `BACKLOG.md`): faster
unpremultiplication of 8-bit pixel buffers (MotionMark Images), lower per-draw dirty rect overhead of canvas 2D, four
GPU memory reductions in the Skia compositor, and two web process memory fixes for back/forward cache and process
swaps.
- Tools: `Tools/Scripts/webkit-gpu-memory` (GPU memory of MiniBrowser vs Chrome/Firefox), `gpu-memory/botrace`.
