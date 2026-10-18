#!/bin/bash
# Baseline before a run, on the main worktree (build it first): MotionMark noise in the headless Weston
# (6 x full MotionMark, count 1 -> metrics/noise.json; each run passes `mm-score.py sanity` and every test's CV must stay
# <= $MM_MAX_NOISE_CV), the layout test subset (--repeat-each=3 -> lt/baseline-subset),
# then the gate worktree build. Each step takes the heavy lock separately. Gates wait for $VIBES_BASELINE_DONE, which
# is written last. Results of an earlier baseline are moved to metrics/old-<timestamp>/ first, so they do not mix in.
# It also pins the gate scripts and agent rules the gates trust (pin_tools in gate-lib.sh).
# usage: nohup setsid vibes/baseline.sh > vibes/logs/baseline.log 2>&1 &
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
source "$here/gate-lib.sh"
V=$VIBES_STATE
log() { echo "- $(date -Is) baseline: $*" >> $V/LOG.md; echo "$*"; }

rm -f "$VIBES_BASELINE_DONE"
pin_tools || exit 1
old=$V/metrics/old-$(date +%Y%m%d-%H%M%S)
shopt -s nullglob
previous=($V/metrics/mm/baseline-* $V/metrics/noise.json $V/metrics/noise.txt $V/lt/baseline-subset*)
shopt -u nullglob
if [ ${#previous[@]} -gt 0 ]; then
    mkdir -p "$old" && mv "${previous[@]}" "$old/" || exit 1
fi

log "start: 6 x full MotionMark on $VIBES_BRANCH $(git -C "$VIBES_MAIN" rev-parse --short HEAD)${previous:+ (previous baseline moved to $old)}"
for i in 1 2 3 4 5 6; do
    $V/heavy.sh mm-baseline $V/mm.sh "$VIBES_MAIN" baseline 1 || exit $?
    # Stop at the first run when the environment is wrong (refresh rate, collapsed scores, WPEPlatform module).
    latest=$(ls -t $V/metrics/mm/baseline-*.json | head -1)
    $V/mm-score.py sanity "$latest" || { log "MotionMark environment check failed: $($V/mm-score.py sanity "$latest" | tail -1)"; exit 1; }
done
if ! $V/mm-score.py noise $V/metrics/mm/baseline-*.json -o $V/metrics/noise.json --max-cv "$MM_MAX_NOISE_CV" > $V/metrics/noise.txt; then
    log "MotionMark too noisy: $(grep -E '^NOISY' $V/metrics/noise.txt) (see metrics/noise.txt)"; exit 1
fi
log "MotionMark done (metrics/noise.json); layout test subset --repeat-each=3 start"
$V/heavy.sh lt-baseline-subset $V/lt-subset.sh "$VIBES_MAIN" baseline-subset --repeat-each=3 || exit $?
[ -f $V/lt/baseline-subset/full_results.json ] || { log "subset run has no full_results.json"; exit 1; }
log "subset done; gate worktree build start"
git -C "$VIBES_GATE" switch -q --detach "$VIBES_BRANCH" || exit 1
$V/heavy.sh build-gate $V/build.sh "$VIBES_GATE" || exit $?
date -Is > "$VIBES_BASELINE_DONE"
log "done"
