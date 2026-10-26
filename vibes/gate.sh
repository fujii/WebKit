#!/bin/bash
# Acceptance gate for a task patch (P and M tracks). Agents must NOT edit this file.
#
# usage:
#   gate.sh paths <rev>                            diff path check only (see check_paths in gate-lib.sh)
#   gate.sh check <wt> <label> <target>...         build, MotionMark A/B, layout test subset
#       Normally run by gate-run.sh: <wt> = $VIBES_GATE (<main>-gate) with the task patch applied on top of $VIBES_BRANCH.
#       <target> = Suite/Test the change should improve, e.g. MotionMark/Paths; none = regressions only (M track).
#       Writes vibes/gates/<label>.txt, whose last line is the verdict. Exit 0 = PASS (still needs the reviewer),
#       1 = REJECT, 3 = BORDERLINE (MotionMark; run the gate again with MM_ROUNDS_TARGET=10 MM_ROUNDS_ALL=6),
#       75 = killed by memwatch.
#   gate.sh merge <label>                          after reviewer approval: fast-forward $VIBES_BRANCH to the commit the gate passed, rebuild
#       $VIBES_BRANCH must not have moved since the gate.
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
source "$here/gate-lib.sh"

case "${1:-}" in
paths)
    check_paths "$2" && echo "paths OK"
    ;;
check)
    gate_check_start "$2" "$3" "gate check"; shift 3
    targets=("$@")
    echo "targets: ${targets[*]:-none}"
    built=$(cat "$MAIN/WebKitBuild/WPE/Release/.vibes-built-commit" 2>/dev/null)
    if [ -z "$built" ] || [[ "$built" == *-dirty ]] || ! git -C "$MAIN" diff --quiet "$built" "$base" -- . ':!vibes' ':!CLAUDE.md' ':!ReadMe.md' ':!.claude'; then
        echo "== rebuilding $B (before build is stale)"
        "$here/heavy.sh" build-main "$here/build.sh" "$MAIN" || { rc=$?; echo "REJECT: main build failed rc=$rc"; exit $rc; }
    fi
    echo "== build $rev"
    "$here/heavy.sh" "build-$label" "$here/build.sh" "$wt" || { rc=$?; echo "REJECT: build failed rc=$rc"; exit $rc; }

    echo "== MotionMark A/B"
    "$here/heavy.sh" "mm-$label-target" "$here/mm-ab.sh" "$MAIN" "$wt" "$label" "${MM_ROUNDS_TARGET:-5}" "${targets[@]}" || { rc=$?; echo "REJECT: MotionMark run failed rc=$rc"; exit $rc; }
    "$here/heavy.sh" "mm-$label-all" "$here/mm-ab.sh" "$MAIN" "$wt" "$label" "${MM_ROUNDS_ALL:-3}" || { rc=$?; echo "REJECT: MotionMark run failed rc=$rc"; exit $rc; }
    if ! "$here/mm-score.py" sanity "$VIBES_STATE/metrics/mm/$label"-[AB]-*.json; then
        echo "REJECT: MotionMark environment check failed (not a measurement; fix the environment and run the gate again)"; exit 1
    fi
    targs=()
    for t in "${targets[@]}"; do targs+=(--target "$t"); done
    "$here/mm-score.py" compare --label "$label" "${targs[@]}"
    mmrc=$?
    case $mmrc in
    0|3) ;;
    1) echo "REJECT: MotionMark"; exit 1 ;;
    *) echo "REJECT: mm-score.py compare failed rc=$mmrc"; exit 1 ;;
    esac

    gate_lt_subset
    if [ $mmrc -eq 3 ]; then echo "BORDERLINE: MotionMark; run the gate again with MM_ROUNDS_TARGET=10 MM_ROUNDS_ALL=6"; exit 3; fi
    echo "PASS $rev (needs reviewer approval, then: gate.sh merge $label)"
    ;;
merge)
    gate_merge "$2" P/M
    ;;
*)
    sed -n '/^# usage:/,/^set -u/{/^#/p}' "$0"; exit 2 ;;
esac
