#!/bin/bash
# Profile one MotionMark subtest with perf (user space, frame pointers). Run through heavy.sh.
# usage: mm-profile.sh <worktree> <label> <Suite/Test>
# Output: metrics/prof/<label>.data; then run prof-report.sh (no heavy lock needed)
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
wt=$(readlink -f "$1"); label=$2; test=$3
out=$VIBES_STATE/metrics/prof/$label
perf record -F 999 -e cycles:u --user-callchains --call-graph=fp -o "$out.data" -- \
    "$here/mm.sh" "$wt" "prof-$label" 1 "$test" > "$out.record.log" 2>&1
rc=$?
echo "mm-profile.sh: rc=$rc data=$out.data (make the report outside the heavy lock: vibes/prof-report.sh <wt> $label)"
exit $rc
