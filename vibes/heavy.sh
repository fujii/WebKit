#!/bin/bash
# Run a heavy job (build, layout tests, MotionMark) under the global lock.
# usage: heavy.sh <name> <command> [args...]
# Exit status: the command's, or 75 if memwatch killed it (retry with half the parallelism).
set -u
source "$(dirname "$(readlink -f "$0")")/limits.sh"
name=$1; shift

# Priority: while $VIBES_PRIORITY exists, only jobs whose name matches the regex in it may start.
# It is checked again after taking the lock: the priority may have been set while this job waited for it.
prio=$VIBES_PRIORITY
allowed() { ! [ -f "$prio" ] || [[ "$name" =~ $(cat "$prio" 2>/dev/null) ]]; }
while :; do
    while ! allowed; do sleep 20; done
    exec 9>"$VIBES_LOCK"
    if ! flock -n 9; then
        echo "heavy.sh: waiting for lock (held by: $(cat "$VIBES_JOB" 2>/dev/null))" >&2
        flock 9
    fi
    allowed && break
    exec 9>&-
done

mw=$(cat "$VIBES_MEMWATCH_PID" 2>/dev/null)
if [ -z "$mw" ] || ! tr '\0' ' ' < "/proc/$mw/cmdline" 2>/dev/null | grep -q 'memwatch\.py'; then
    echo "heavy.sh: memwatch.py is not running; refusing to start $name" >&2
    exit 76
fi

while :; do
    avail_gb=$(awk '/MemAvailable/ {print int($2/1048576)}' /proc/meminfo)
    [ "$avail_gb" -ge "$MEM_START_GB" ] && break
    echo "heavy.sh: MemAvailable ${avail_gb}GB < ${MEM_START_GB}GB, waiting" >&2
    sleep 30
done

echo 1000 > /proc/self/oom_score_adj
rm -f "$VIBES_MEMKILL"
id=$(date +%s%N)
start=$(date -Is)
# The new session id equals the pid of the bash -c process; memwatch kills by session,
# process tree and VIBES_HEAVY_ID environment variable.
VIBES_HEAVY_ID=$id setsid bash -c 'echo "$$ '"$id"' '"$name"'" > "'"$VIBES_JOB"'"; exec "$@"' heavy "$@" 9>&- &
wait $!
rc=$?
rm -f "$VIBES_JOB"
if [ -f "$VIBES_MEMKILL" ] && grep -q "$id" "$VIBES_MEMKILL"; then
    echo "heavy.sh: $name was killed by memwatch: $(cat "$VIBES_MEMKILL")" >&2
    rc=75
fi
echo "$(date -Is) heavy $name rc=$rc start=$start" >> "$VIBES_STATE/logs/heavy.log"
exit $rc
