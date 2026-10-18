# Paths and resource limits for WPE Vibes. Agents must NOT edit this file.
# Every value with a default (${X:-...}) can be overridden in the environment or in vibes/limits.local.sh (not
# committed; sourced first, so it wins over the environment and the derived paths follow it).

_vibes_dir=$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")
[ -f "$_vibes_dir/limits.local.sh" ] && source "$_vibes_dir/limits.local.sh"

# The main worktree is the one that has the branch $VIBES_BRANCH checked out, so the scripts find it from any
# worktree. Worker and gate worktrees are its siblings: <main>-w1, <main>-gate.
VIBES_BRANCH=${VIBES_BRANCH:-wpe-vibes}
VIBES_MAIN=${VIBES_MAIN:-$(git -C "$_vibes_dir" worktree list --porcelain |
    awk -v b="refs/heads/$VIBES_BRANCH" '/^worktree / {wt = substr($0, 10)} $0 == "branch " b {print wt; exit}')}
VIBES_MAIN=${VIBES_MAIN:-$(dirname "$_vibes_dir")}
VIBES_W1=${VIBES_W1:-$VIBES_MAIN-w1}             # the worker worktree
VIBES_GATE=${VIBES_GATE:-$VIBES_MAIN-gate}       # the gate worktree (detached HEAD)
VIBES_STATE=$VIBES_MAIN/vibes                    # shared state (LOG.md, metrics/) lives in the main worktree
VIBES_RUN=$VIBES_STATE/run                       # runtime control files (not committed)
VIBES_LOCK=$VIBES_RUN/heavy.lock                 # one heavy job at a time
VIBES_JOB=$VIBES_RUN/heavy.job                   # "<sid> <id> <name>" of the running heavy job
VIBES_MEMKILL=$VIBES_RUN/memkill                 # written by memwatch when it kills a job
VIBES_MEMWATCH_PID=$VIBES_RUN/memwatch.pid       # pid of the running memwatch.py
VIBES_PRIORITY=$VIBES_RUN/priority               # regex of the heavy job names allowed to start, while it exists
VIBES_GATE_LOCK=$VIBES_RUN/gate.lock             # one gate at a time (they share the gate worktree)
VIBES_TOOLS_PIN=$VIBES_RUN/tools.pin             # "<commit> <limits.local.sh hash>" the gates trust (gate-lib.sh)
VIBES_STOP=$VIBES_RUN/stop                       # loop.sh stops after the current iteration while it exists
VIBES_BASELINE_DONE=$VIBES_STATE/metrics/baseline.done   # written by baseline.sh when it finished; gates wait for it
mkdir -p "$VIBES_RUN" "$VIBES_STATE/logs" "$VIBES_STATE/metrics/mm" "$VIBES_STATE/metrics/prof"

_mem_total_gb=$(awk '/MemTotal/ {print int($2/1048576)}' /proc/meminfo)
BUILD_JOBS=${BUILD_JOBS:-$(( $(nproc) > 1 ? $(nproc) / 2 : 1 ))}                    # ninja -j
LT_CHILD_PROCESSES=${LT_CHILD_PROCESSES:-$(( $(nproc) > 3 ? $(nproc) / 4 : 1 ))}    # run-webkit-tests --child-processes

MEM_START_GB=${MEM_START_GB:-$(( _mem_total_gb * 60 / 100 ))}    # wait for MemAvailable >= this before a heavy job
MEM_KILL_GB=${MEM_KILL_GB:-$(( _mem_total_gb * 20 / 100 ))}      # memwatch kills the heavy job below this
PSI_FULL_AVG10_KILL=${PSI_FULL_AVG10_KILL:-5}       # memwatch kills when /proc/pressure/memory full avg10 exceeds this
SWAPIN_PAGES_PER_SEC=${SWAPIN_PAGES_PER_SEC:-2000}  # memwatch kills when pswpin rate exceeds this ...
SWAPIN_SECONDS=${SWAPIN_SECONDS:-5}                 # ... for this many consecutive seconds

MM_MAX_NOISE_CV=${MM_MAX_NOISE_CV:-0.03}           # baseline.sh fails when a MotionMark test's CV exceeds this

# CPUs MotionMark is pinned to (taskset -c list); empty = no pinning. Pinning to the CPUs of one L3 cache lowers the
# noise on multi-CCD CPUs (e.g. 0-7,32-39 on a 32-core Zen).
MM_TASKSET_DEFAULT=${MM_TASKSET_DEFAULT:-}
# CPUs prof-report.sh runs perf report on (taskset -c list), e.g. CPUs away from a running MotionMark; empty = any.
PROF_TASKSET=${PROF_TASKSET:-}

# Claude Code plugin directory passed to the loop.sh sessions (optional; e.g. one providing the webkit-dev skills).
VIBES_PLUGIN_DIR=${VIBES_PLUGIN_DIR:-}

unset _vibes_dir _mem_total_gb
