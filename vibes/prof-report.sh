#!/bin/bash
# Text report of a perf profile from mm-profile.sh. Light job: no heavy lock needed.
# usage: prof-report.sh <worktree> <label>   -> metrics/prof/<label>.txt (self time by thread, dso, symbol)
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
wt=$(readlink -f "$1"); label=$2
out=$VIBES_STATE/metrics/prof/$label
prefix=()
[ -n "$PROF_TASKSET" ] && prefix=(taskset -c "$PROF_TASKSET")
cd "$wt"
"${prefix[@]}" nice Tools/Scripts/container-sdk-rootdir-wrapper perf report -i "$out.data" --no-children --no-inline -g none \
    --sort comm,dso,sym --percent-limit 0.2 --stdio > "$out.txt" 2> "$out.report.log"
echo "prof-report.sh: report=$out.txt"
