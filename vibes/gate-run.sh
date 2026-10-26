#!/bin/bash
# Orchestrator helper: apply a task's patch (vibes/patches/<ID>.patch) onto $VIBES_BRANCH in the gate worktree and
# run the gate.
# usage: [GATE_SCRIPT=qgate.sh] [MM_ROUNDS_TARGET=n MM_ROUNDS_ALL=n] gate-run.sh <label> "<patch>|<commit>..." <gate check args after the label>...
#   Each <patch> is a file made by `git format-patch` (applied with `git am -3`); a <commit> is cherry-picked. The gated
#   commit is kept as refs/vibes/gates/<label>, so `gate.sh merge <label>` can fast-forward $VIBES_BRANCH to it.
# Gates run one at a time (they share the gate worktree): a second gate-run.sh waits for the first. They also wait
# for baseline.sh to finish. gates/<label>.txt has no verdict while the gate waits or runs; its last line is then
# "PASS <commit>", "REJECT: ..." or "BORDERLINE: ...". A label is used once.
set -u
source "$(dirname "$(readlink -f "$0")")/limits.sh"
V=$VIBES_STATE
G=$VIBES_GATE
B=$VIBES_BRANCH
label=$1; commits=$2; shift 2
out=$V/gates/$label.txt
mkdir -p "$V/gates"
[ -e "$out" ] && { echo "gate $label exists already; use a new label" >&2; exit 2; }
log() { echo "- $(date -Is) gate $label $*" >> $V/LOG.md; }
reject() { echo "REJECT: $*" >> "$out"; log "REJECT: $*"; exit 1; }
echo "gate-run $label: waiting for the baseline and for a running gate ($(date -Is))" > "$out"
until [ -f "$VIBES_BASELINE_DONE" ]; do sleep 30; done
exec 8>"$VIBES_GATE_LOCK"
flock 8
# Let the gate jobs go first; restore the previous priority afterwards.
P=$VIBES_PRIORITY
prev=$(cat $P 2>/dev/null)
echo '^(build-gate|build-main|build-g[0-9]|mm-g[0-9]|lt-g[0-9]|api-g[0-9])' > $P
trap 'if [ -n "$prev" ]; then echo "$prev" > $P; else rm -f $P; fi' EXIT
# The gate worktree only serves gates: drop whatever an interrupted gate left there.
git -C $G cherry-pick --abort > /dev/null 2>&1
git -C $G am --abort > /dev/null 2>&1
git -C $G switch -q --detach --discard-changes "$B" || reject "cannot switch $G to $B"
for c in $commits; do
    if [ -f "$c" ]; then
        echo "patch $c sha256 $(sha256sum < "$c" | cut -c1-16)" >> "$out"
        git -C $G am -q -3 --keep-non-patch "$(readlink -f "$c")" > /dev/null || { git -C $G am --abort; reject "git am $c failed"; }
    else
        git -C $G cherry-pick $c > /dev/null || { git -C $G cherry-pick --abort; reject "cherry-pick $c failed"; }
    fi
done
git -C $G update-ref "refs/vibes/gates/$label" HEAD
log "start: $(git -C $G log --oneline "$B..HEAD" | tr '\n' ';')"
$V/${GATE_SCRIPT:-gate.sh} check $G "$label" "$@" 8>&-
rc=$?
case "$(tail -1 "$out")" in
"PASS "*|REJECT*|BORDERLINE*) ;;
*) echo "REJECT: ${GATE_SCRIPT:-gate.sh} exited with rc=$rc without a verdict" >> "$out" ;;
esac
log "rc=$rc: $(tail -1 "$out")"
exit $rc
