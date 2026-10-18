#!/bin/bash
# Run layout tests of a worktree's WPE Release build. Run through heavy.sh.
# usage: lt-run.sh <worktree> <result-name> [run-webkit-tests args / tests...]
# Results: $VIBES_STATE/lt/<result-name>/full_results.json
set -u
source "$(dirname "$(readlink -f "$0")")/limits.sh"
wt=$(readlink -f "$1"); name=$2; shift 2
out=$VIBES_STATE/lt/$name
mkdir -p "$VIBES_STATE/lt"
cd "$wt"
Tools/Scripts/run-webkit-tests --wpe --release --no-show-results --clobber-old-results \
    --child-processes=$LT_CHILD_PROCESSES --results-directory="$out" "$@" > "$out.log" 2>&1
rc=$?
echo "lt-run.sh: rc=$rc (number of unexpected failures) results=$out log=$out.log"
tail -30 "$out.log" | grep -vE '^\s*$' | tail -15
exit 0
