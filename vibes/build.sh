#!/bin/bash
# Release build of a worktree. Run through heavy.sh: vibes/heavy.sh build-<wt> vibes/build.sh <worktree>
# usage: build.sh <worktree-dir> [extra build-webkit args...]
set -u
source "$(dirname "$(readlink -f "$0")")/limits.sh"
wt=$(readlink -f "$1"); shift
log=$VIBES_STATE/logs/build-$(basename "$wt")-$(date +%Y%m%d-%H%M%S).log
cd "$wt"
unset BUILD_WEBKIT_PRE_SCRIPT   # knocks the remote ccache, which is disabled
NUMBER_OF_PROCESSORS=$BUILD_JOBS Tools/Scripts/build-webkit --wpe --release "$@" > "$log" 2>&1
rc=$?
if [ $rc -eq 0 ]; then
    c=$(git rev-parse HEAD); git diff --quiet HEAD -- . ':!vibes' ':!CLAUDE.md' ':!ReadMe.md' ':!.claude' || c="$c-dirty"
    echo "$c" > WebKitBuild/WPE/Release/.vibes-built-commit
fi
echo "build.sh: rc=$rc log=$log"
[ $rc -ne 0 ] && grep -E 'error:|FAILED:' "$log" | head -20
exit $rc
