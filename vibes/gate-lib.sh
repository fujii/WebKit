# Shared by gate.sh and qgate.sh (and the tools pin of baseline.sh and loop.sh). Agents must NOT edit this file.
# Source it after limits.sh, with $here set to the directory of the calling script.

MAIN=$VIBES_MAIN
B=$VIBES_BRANCH
GATES=$VIBES_STATE/gates
mkdir -p "$GATES"

# What the gates trust: their scripts, the agents' rules and settings. They must match the commit pinned at the
# start of the run ($VIBES_TOOLS_PIN), so committing a change to them does not get past the gates either.
PROTECTED=(':(glob)vibes/*.sh' ':(glob)vibes/*.py' ':(glob)vibes/*.ini' ':(glob)vibes/*.json' vibes/ITERATION.md
    CLAUDE.md ':(glob).claude/**')

limits_local_hash() {
    if [ -f "$VIBES_STATE/limits.local.sh" ]; then sha256sum < "$VIBES_STATE/limits.local.sh" | cut -c1-16; else echo none; fi
}

# Files under the protected paths that git does not track (e.g. .claude/settings.local.json), except limits.local.sh.
untracked_protected() {
    git -C "$MAIN" ls-files --others -- "${PROTECTED[@]}" | grep -vx 'vibes/limits.local.sh'
}

# Pins the protected files of the main worktree's HEAD and the current limits.local.sh. Called when a run starts.
pin_tools() {
    if ! git -C "$MAIN" diff --quiet HEAD -- "${PROTECTED[@]}" || [ -n "$(untracked_protected)" ]; then
        echo "cannot pin the tools: commit the changes to them first:"
        git -C "$MAIN" status --short -- "${PROTECTED[@]}"
        return 1
    fi
    local head
    head=$(git -C "$MAIN" rev-parse HEAD)
    echo "$head $(limits_local_hash)" > "$VIBES_TOOLS_PIN"
    echo "- $(date -Is) tools pinned at ${head:0:12} (limits.local.sh $(limits_local_hash))" >> "$VIBES_STATE/LOG.md"
}

# The gate scripts that run are the main worktree's copies, and they must be the pinned ones.
check_scripts() {
    if [ "$here" != "$(readlink -f "$VIBES_STATE")" ]; then
        echo "run the gate from the main worktree ($VIBES_STATE), not $here"; return 1
    fi
    local pin hash extra
    read -r pin hash 2>/dev/null < "$VIBES_TOOLS_PIN" || { echo "no $VIBES_TOOLS_PIN; baseline.sh or loop.sh writes it"; return 1; }
    if ! git -C "$MAIN" diff --quiet "$pin" -- "${PROTECTED[@]}"; then
        echo "the gate scripts or agent rules in $MAIN differ from the pinned ${pin:0:12}:"
        git -C "$MAIN" diff --stat "$pin" -- "${PROTECTED[@]}"
        return 1
    fi
    extra=$(untracked_protected)
    [ -z "$extra" ] || { echo "untracked files under the protected paths: $extra"; return 1; }
    [ "$(limits_local_hash)" = "$hash" ] || { echo "vibes/limits.local.sh changed since it was pinned"; return 1; }
}

# Copies all output to the gate result file; the file is complete when the script exits.
start_gate_log() {
    exec > >(tee "$1") 2>&1
    GATE_TEE_PID=$!
    trap 'exec >&- 2>&-; wait $GATE_TEE_PID' EXIT
}

# Exit status 0 when the diff of <path> only removes lines (a kept line may lose its trailing comma: JSON).
only_removes_lines() {   # <rev> <path>
    git -C "$MAIN" diff -U0 "$B...$1" -- "$2" | awk '
        /^(\+\+\+|---) / { next }
        /^-/ { removed[substr($0, 2)] = 1; next }
        /^\+/ { line = substr($0, 2); if (line !~ /^[[:space:]]*$/ && !((line ",") in removed)) gained = 1 }
        END { exit gained }'
}

# Exit status 0 when the test of a new -expected file is new too: a new expected result of an existing test (e.g. a
# platform/wpe one) is a rebaseline.
test_added_for_expected() {   # <expected-file> <added files, one per line>
    local t=${1#LayoutTests/} f
    t=${t#platform/*/}
    t=LayoutTests/${t%-expected*}
    while IFS= read -r f; do
        case "$f" in "$t".*) case "$f" in *-expected*) ;; *) return 0 ;; esac ;; esac
    done <<< "$2"
    return 1
}

check_paths() {   # <rev>
    local rev=$1 bad=0 status path rest p added
    path_reject() { echo "PATH REJECT ($status): $p${1:+ ($1)}"; bad=1; }
    added=$(git -C "$MAIN" diff --name-only --diff-filter=A "$B...$rev")
    while IFS=$'\t' read -r status path rest; do
        for p in "$path" ${rest:+"$rest"}; do
            case "$p" in
            LayoutTests/TestExpectations|LayoutTests/platform/wpe/TestExpectations|LayoutTests/platform/glib/TestExpectations|Tools/TestWebKitAPI/glib/TestExpectations.json|WebDriverTests/TestExpectations.json)
                if [ "$status" != M ] || ! only_removes_lines "$rev" "$p"; then
                    path_reject "only removing the expectations of fixed tests is allowed"
                fi ;;
            LayoutTests/*-expected.*|LayoutTests/*-expected-mismatch.*)
                if [ "$status" != A ]; then
                    path_reject "existing tests cannot change"
                elif ! test_added_for_expected "$p" "$added"; then
                    path_reject "new expected result of an existing test: a rebaseline"
                fi ;;
            LayoutTests/*|Tools/TestWebKitAPI/Tests/*|WebDriverTests/*)
                [ "$status" = A ] || path_reject "existing tests cannot change" ;;
            vibes/*|.claude/*|CLAUDE.md|PerformanceTests/*|Tools/Scripts/webkitpy/*|Tools/Scripts/webkitperl/*|Tools/Scripts/webkitdirs.pm|Tools/Scripts/build-webkit|Tools/Scripts/run-*|Tools/glib/*|Tools/MiniBrowser/*|Source/cmake/*|CMakeLists.txt)
                path_reject ;;
            esac
        done
    done < <(git -C "$MAIN" diff --name-status -M "$B...$rev")
    return $bad
}

# Common start of "check": the result file, the scripts, a clean committed tree based on $B HEAD, the paths.
# Sets wt, label, rev, base.
gate_check_start() {   # <wt> <label> <title>
    wt=$(readlink -f "$1"); label=$2
    start_gate_log "$GATES/$label.txt"
    echo "$3 $label: worktree=$wt start=$(date -Is)"
    check_scripts || { echo "REJECT: gate scripts"; exit 1; }
    rev=$(git -C "$wt" rev-parse HEAD)   # a commit; the gate worktree uses a detached HEAD
    if ! git -C "$wt" diff --quiet HEAD; then echo "REJECT: uncommitted changes in $wt"; exit 1; fi
    base=$(git -C "$MAIN" rev-parse "$B")
    if [ "$(git -C "$wt" merge-base HEAD "$base")" != "$base" ]; then
        echo "REJECT: $rev is not based on $B HEAD $base; rebase it first"; exit 1
    fi
    git -C "$MAIN" log --oneline "$B..$rev"
    check_paths "$rev" || { echo "REJECT: path check"; exit 1; }
}

# Layout test subset of $wt: no new failures against the baseline (new failures are rerun 3 times first).
gate_lt_subset() {
    echo "== layout tests (subset)"
    "$here/heavy.sh" "lt-$label" "$here/lt-subset.sh" "$wt" "gate-$label" || { rc=$?; echo "REJECT: layout tests run failed rc=$rc"; exit $rc; }
    local base_lt=() ignore=() d
    for d in "$VIBES_STATE"/lt/baseline-subset*; do [ -f "$d/full_results.json" ] && base_lt+=("$d"); done
    [ ${#base_lt[@]} -gt 0 ] || { echo "REJECT: no baseline subset results"; exit 1; }
    [ -f "$VIBES_STATE/metrics/lt-flaky.txt" ] && ignore=(--ignore "$VIBES_STATE/metrics/lt-flaky.txt")
    if ! "$here/lt-diff.py" "${ignore[@]}" "${base_lt[@]}" -- "$VIBES_STATE/lt/gate-$label"; then
        echo "== rerunning new failures with --repeat-each=3"
        "$here/lt-diff.py" "${ignore[@]}" "${base_lt[@]}" -- "$VIBES_STATE/lt/gate-$label" | awk '/^NEW FAILURE/ {print $3}' > "$GATES/$label-newfail.txt"
        [ -s "$GATES/$label-newfail.txt" ] || { echo "REJECT: lt-diff failed without a list of new failures"; exit 1; }
        "$here/heavy.sh" "lt-$label-rerun" "$here/lt-run.sh" "$wt" "gate-$label-rerun" --repeat-each=3 --no-retry-failures --test-list="$GATES/$label-newfail.txt"
        "$here/lt-diff.py" "${ignore[@]}" "${base_lt[@]}" -- "$VIBES_STATE/lt/gate-$label-rerun" || { echo "REJECT: new layout test failures reproduce"; exit 1; }
    fi
}

# After reviewer approval: fast-forward $B to the commit gate <label> passed, rebuild. $B must not have moved since
# the gate.
gate_merge() {   # <label> <kind>
    local label=$1 kind=$2 gated
    check_scripts || exit 1
    gated=$(awk '/^PASS / {print $2; exit}' "$GATES/$label.txt" 2>/dev/null)
    [ -n "$gated" ] || { echo "gate $label did not pass"; exit 1; }
    git -C "$MAIN" cat-file -e "$gated^{commit}" || { echo "the commit $gated of gate $label is gone; run the gate again"; exit 1; }
    check_paths "$gated" || exit 1
    cd "$MAIN" || exit 1
    git diff --quiet HEAD -- . ':!vibes' || { echo "$B has uncommitted source changes"; exit 1; }
    git merge-base --is-ancestor "$B" "$gated" || { echo "$B moved since gate $label ran on it; run the gate again"; exit 1; }
    git merge -q --ff-only "$gated" || { echo "fast-forward to $gated failed"; exit 1; }
    echo "$(date -Is) merged $label ($kind): $(git log --oneline -1)" >> "$VIBES_STATE/logs/merges.log"
    "$here/heavy.sh" build-main "$here/build.sh" "$MAIN"
}
