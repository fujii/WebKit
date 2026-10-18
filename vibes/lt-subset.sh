#!/bin/bash
# Fast rendering-related layout test subset (gate). usage: lt-subset.sh <worktree> <result-name> [extra args]
set -u
here=$(dirname "$(readlink -f "$0")")
wt=$1; name=$2; shift 2
exec "$here/lt-run.sh" "$wt" "$name" "$@" \
    compositing fast/canvas css3 animations webanimations svg \
    imported/w3c/web-platform-tests/html/canvas \
    imported/w3c/web-platform-tests/css/css-transforms \
    imported/w3c/web-platform-tests/css/filter-effects
