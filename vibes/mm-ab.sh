#!/bin/bash
# Interleaved A/B MotionMark measurement (A = before, B = after), n rounds of A then B.
# Run through heavy.sh: vibes/heavy.sh mm-ab vibes/mm-ab.sh <before-wt> <after-wt> <label> <n> <subtest...>
# Then: vibes/mm-score.py compare --label <label>
set -u
here=$(dirname "$(readlink -f "$0")")
before=$1; after=$2; label=$3; n=$4; shift 4
for i in $(seq 1 "$n"); do
    "$here/mm.sh" "$before" "$label-A" 1 "$@" || exit $?
    "$here/mm.sh" "$after" "$label-B" 1 "$@" || exit $?
done
