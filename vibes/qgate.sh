#!/bin/bash
# Quality (Q track) acceptance gate: fixes of failing, crashing or flaky tests. Agents must NOT edit this file.
#
# usage:
#   qgate.sh check <wt> <label> lt <test>...                  layout tests, run with --repeat-each=5
#   qgate.sh check <wt> <label> api <run-wpe-tests args>...   API tests, run 3 times
#   qgate.sh check <wt> <label> webdriver <run-webdriver-tests args>...   WebDriver tests, run 3 times
#       Normally run by gate-run.sh: <wt> = $VIBES_GATE (<main>-gate) with the task patch applied on top of $VIBES_BRANCH.
#       Requires: builds; every target passes every time (layout tests: every target runs at least one test;
#       API/WebDriver: the targets are named and no expectation of them allows anything but PASS on WPE, so a
#       still failing target fails the run); layout test subset has no new failures.
#       Writes vibes/gates/<label>.txt, whose last line is the verdict. Exit 0 = PASS (still needs the reviewer),
#       1 = REJECT, 75 = killed by memwatch.
#   qgate.sh merge <label>             after reviewer approval: fast-forward $VIBES_BRANCH to the commit the gate passed, rebuild
#       $VIBES_BRANCH must not have moved since the gate.
#
# Paths: see check_paths in gate-lib.sh. TestExpectations files may only lose lines (removing the expectation of a
# fixed test); existing tests and expected results cannot change.
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
source "$here/gate-lib.sh"

targets_passed() {   # <results-dir> <test>...: every target ran, and every result of every target test is PASS
    python3 - "$@" <<'EOF'
import json, os, sys
d, tests = sys.argv[1], sys.argv[2:]
t = open(os.path.join(d, 'full_results.json')).read().strip()
if t.startswith('ADD_RESULTS('):
    t = t[len('ADD_RESULTS('):t.rfind(')')]
data = json.loads(t)
def walk(node, prefix, leaf, out):
    for k, v in node.items():
        n = f'{prefix}/{k}' if prefix else k
        if isinstance(v, dict) and leaf in v:
            out[n] = v[leaf]
        elif isinstance(v, dict):
            walk(v, n, leaf, out)
    return out
def matches(name, x):
    return name == x or name.startswith(x.rstrip('/') + '/')
# full_results.json omits the tests that passed as expected; stats.json has every test that ran.
results = {n: a.split() for n, a in walk(data['tests'], '', 'actual', {}).items()}
with open(os.path.join(d, 'stats.json')) as f:
    ran = [n for n in walk(json.load(f), '', 'results', {}) if any(matches(n, x) for x in tests)]
bad = 0
if data.get('interrupted'):
    print('TARGET RUN INTERRUPTED'); bad = 1
for x in tests:
    if not any(matches(n, x) for n in ran):
        print(f'TARGET DID NOT RUN: {x}'); bad = 1
for name, actual in sorted(results.items()):
    if any(matches(name, x) for x in tests) and any(a != 'PASS' for a in actual):
        print(f'TARGET NOT FIXED: {name}: {" ".join(actual)}'); bad = 1
print(f'targets: {len(tests)} given, {len(ran)} tests ran, {"REJECT" if bad else "all passed"}')
sys.exit(bad)
EOF
}

# run-wpe-tests and run-webdriver-tests exit with 0 when a test fails as its expectation says, and skip the tests
# expected to SKIP. So the targets must have no WPE expectation but PASS: then a run passes only if they pass.
targets_expected_to_pass() {   # <wt> api|webdriver <runner args>...
    python3 - "$@" <<'EOF'
import json, os, sys
wt, kind, args = sys.argv[1], sys.argv[2], sys.argv[3:]
path = {'api': 'Tools/TestWebKitAPI/glib/TestExpectations.json', 'webdriver': 'WebDriverTests/TestExpectations.json'}[kind]
with open(os.path.join(wt, path)) as f:
    expectations = json.load(f)
targets = [a for a in args if not a.startswith('-')]
if not targets:
    print(f'TARGETS NOT NAMED: give the {kind} tests the change fixes'); sys.exit(1)

def statuses(expected):   # every WPE (or all-port) status list, whatever the build type or architecture
    return [v.get('status', ['PASS']) for k, v in expected.items() if k == 'all' or k.split('@')[0].split(':')[0] in ('wpe', 'all')]

bad = 0
for target in targets:
    if kind == 'api':   # TestName, path/to/TestName or TestName:/sub/test
        test, _, subtest = target.partition(':')
        keys = [os.path.basename(test)]
    else:               # a test file or directory, relative to WebDriverTests/
        test, _, subtest = target.partition('::')
        test = test.removeprefix('WebDriverTests/').rstrip('/')
        keys = [k for k in expectations if k == test or k.startswith(test + '/')]
    for key in keys:
        entry = expectations.get(key, {})
        found = [(key, s) for s in statuses(entry.get('expected', {}))]
        for name, sub in entry.get('subtests', {}).items():
            if not subtest or name == subtest:
                found += [(f'{key}:{name}', s) for s in statuses(sub.get('expected', {}))]
        for name, s in found:
            if s != ['PASS']:
                print(f'TARGET EXPECTED NOT TO PASS: {name}: {" ".join(s)} (remove the expectation of the fixed test)'); bad = 1
print(f'target expectations: {len(targets)} targets, {"REJECT" if bad else "only PASS"}')
sys.exit(bad)
EOF
}

case "${1:-}" in
check)
    gate_check_start "$2" "$3" "qgate check"; kind=$4; shift 4
    echo "kind=$kind args=$*"
    echo "== build"
    "$here/heavy.sh" "build-$label" "$here/build.sh" "$wt" || { rc=$?; echo "REJECT: build failed rc=$rc"; exit $rc; }
    echo "== targets ($kind)"
    case "$kind" in
    lt)
        "$here/heavy.sh" "lt-$label-target" "$here/lt-run.sh" "$wt" "qgate-$label-target" --repeat-each=5 --no-retry-failures "$@" \
            || { rc=$?; echo "REJECT: layout test run failed rc=$rc"; exit $rc; }
        targets_passed "$VIBES_STATE/lt/qgate-$label-target" "$@" || { echo "REJECT: targets"; exit 1; }
        ;;
    api|webdriver)
        targets_expected_to_pass "$wt" "$kind" "$@" || { echo "REJECT: targets"; exit 1; }
        cmd=(Tools/Scripts/run-wpe-tests --release)
        [ "$kind" = webdriver ] && cmd=(Tools/Scripts/run-webdriver-tests --wpe --release)
        for i in 1 2 3; do
            log=$GATES/$label-target-$i.log
            "$here/heavy.sh" "api-$label-target" bash -c 'cd "$1"; shift; "$@"' x "$wt" "${cmd[@]}" "$@" > "$log" 2>&1
            rc=$?
            tail -15 "$log"
            [ $rc -eq 0 ] || { echo "REJECT: $kind run $i rc=$rc (log $log)"; exit 1; }
            if grep -qE 'No tests found|Collected 0 test files|Ran 0 tests' "$log"; then
                echo "REJECT: $kind run $i ran no tests (log $log)"; exit 1
            fi
        done
        ;;
    *) echo "REJECT: unknown kind $kind"; exit 2 ;;
    esac
    gate_lt_subset
    echo "PASS $rev (needs reviewer approval, then: qgate.sh merge $label)"
    ;;
merge)
    gate_merge "$2" Q
    ;;
*)
    sed -n '/^# usage:/,/^set -u/{/^#/p}' "$0"; exit 2 ;;
esac
