#!/bin/bash
# Run MotionMark 1.3.1 on a worktree's WPE Release MiniBrowser (fullscreen) in the headless Weston of
# weston-vibes.sh (1920x1080, ~58.5 fps). MM_DISPLAY=host uses the host display instead (do not touch it then).
# Run through heavy.sh.
# usage: mm.sh <worktree> <label> <count> [subtest...]     subtest = Suite/Test, e.g. MotionMark/Multiply
# Output: $VIBES_STATE/metrics/mm/<label>-<timestamp>.json (+ .meta with commit, settings, and the WPEPlatform
# modules the MiniBrowser processes mapped: checked by `mm-score.py sanity`)
set -u
source "$(dirname "$(readlink -f "$0")")/limits.sh"
wt=$(readlink -f "$1"); label=$2; count=$3; shift 3
# Without WPE_PLATFORM, WPEPlatform picks the highest priority module: /usr/local/lib/wpe-platform-2.0/modules has
# wpe-platform-gtk (priority 200 > wayland 0), so runs 1-3 measured wpe-platform-gtk. Measure the built-in Wayland one.
export WPE_PLATFORM=${WPE_PLATFORM:-wayland}
MM_TASKSET=${MM_TASKSET-$MM_TASKSET_DEFAULT}   # MM_TASKSET= (empty) disables pinning
stamp=$(date +%Y%m%d-%H%M%S)
out=$VIBES_STATE/metrics/mm/$label-$stamp.json
if [ "${MM_DISPLAY:-headless}" = host ]; then
    display=host
else
    WAYLAND_DISPLAY=$("$(dirname "$(readlink -f "$0")")/weston-vibes.sh") || exit 1
    export WAYLAND_DISPLAY; unset DISPLAY
    display=headless-$WAYLAND_DISPLAY
fi
cd "$wt"
{
    echo "worktree=$wt"
    echo "commit=$(git rev-parse HEAD)"
    echo "count=$count subtests=$*"
    echo "taskset=${MM_TASKSET:-none}"
    echo "display=$display"
    echo "wpe_platform=$WPE_PLATFORM"
    echo "start=$(date -Is)"
} > "$out.meta"
sub=()
[ $# -gt 0 ] && sub=(--subtests "$@")
prefix=()
[ -n "${MM_TASKSET:-}" ] && prefix=(taskset -c "$MM_TASKSET")
# Record which WPEPlatform modules MiniBrowser loads: a module from a modules directory (e.g. wpe-platform-gtk)
# instead of the built-in Wayland one changes what is measured.
maps=$out.maps
(
    while :; do
        for p in $(pgrep -x MiniBrowser); do
            echo seen
            grep -oE '/wpe-platform-[0-9.]+/modules/[^ ]+\.so' "/proc/$p/maps" 2>/dev/null
        done
        sleep 5
    done
) > "$maps" &
watcher=$!
"${prefix[@]}" Tools/Scripts/run-benchmark --plan motionmark1.3.1 --browser minibrowser-wpe \
    --count "$count" --output-file "$out" "${sub[@]}" > "$out.log" 2>&1
rc=$?
kill $watcher 2>/dev/null; wait $watcher 2>/dev/null
modules=$(grep -v '^seen$' "$maps" | sort -u | xargs -r -n1 basename | tr '\n' ' ')
{
    echo "minibrowser_seen=$(grep -q '^seen$' "$maps" && echo yes || echo no)"
    echo "platform_modules=${modules:-none}"
    echo "end=$(date -Is) rc=$rc"
} >> "$out.meta"
rm -f "$maps"
echo "mm.sh: rc=$rc out=$out"
[ $rc -ne 0 ] && tail -20 "$out.log"
exit $rc
