#!/usr/bin/env python3
# Compare layout test results against baseline runs. Agents must NOT edit this file.
# usage: lt-diff.py [--ignore FILE] <baseline-results-dir>... -- <new-results-dir>
#   A results dir is one written by lt-run.sh (contains full_results.json).
#   Exit status 1 when there are new failures or the new run was interrupted (tests that did not run look passed).
#
# full_results.json omits tests that were expected to PASS and passed, so a test missing
# from it passed (or did not run). A test is:
#   - known-bad: it has any non-PASS result in any baseline run (known failure or flaky),
#                or is listed in --ignore files.
#   - a new failure: every result of it in the new run is non-PASS, and it is not known-bad,
#                or it newly CRASHes/TIMEOUTs although the baseline failed differently.
#   - newly flaky: mixed results in the new run and not known-bad (reported, not fatal).
import json
import os
import sys

BAD_KINDS = {'CRASH', 'TIMEOUT'}


def load(results_dir):
    path = os.path.join(results_dir, 'full_results.json')
    with open(path) as f:
        text = f.read().strip()
    if text.startswith('ADD_RESULTS('):
        text = text[len('ADD_RESULTS('):]
        text = text[:text.rfind(')')]
    data = json.loads(text)
    out = {}

    def walk(node, prefix):
        for key, value in node.items():
            name = f'{prefix}/{key}' if prefix else key
            if isinstance(value, dict) and 'actual' in value:
                out[name] = value['actual'].split()
            elif isinstance(value, dict):
                walk(value, name)
    walk(data['tests'], '')
    return out, data.get('interrupted', False)


def main(argv):
    ignore = set()
    args = argv[1:]
    while args and args[0] == '--ignore':
        with open(args[1]) as f:
            ignore |= {line.split('#')[0].strip() for line in f if line.split('#')[0].strip()}
        args = args[2:]
    if '--' not in args:
        print(__doc__ if __doc__ else 'usage: lt-diff.py [--ignore FILE] <baseline>... -- <new>')
        return 2
    sep = args.index('--')
    baselines = [load(d)[0] for d in args[:sep]]
    new, interrupted = load(args[sep + 1])

    known = {}
    for b in baselines:
        for test, actual in b.items():
            if any(a != 'PASS' for a in actual):
                known.setdefault(test, set()).update(actual)

    new_failures, new_flaky = [], []
    for test, actual in sorted(new.items()):
        if test in ignore:
            continue
        failing = [a for a in actual if a != 'PASS']
        if not failing:
            continue
        consistent = len(failing) == len(actual)
        if test not in known:
            (new_failures if consistent else new_flaky).append((test, actual))
        elif consistent and (set(failing) & BAD_KINDS) - known[test] and 'PASS' not in known[test]:
            new_failures.append((test, actual))

    for test, actual in new_failures:
        print(f'NEW FAILURE  {test}  {" ".join(actual)}')
    for test, actual in new_flaky:
        print(f'NEW FLAKY    {test}  {" ".join(actual)}')
    print(f'lt-diff: {len(new_failures)} new failures, {len(new_flaky)} newly flaky '
          f'(baseline known-bad: {len(known)}, ignored: {len(ignore)})')
    if interrupted:
        print(f'lt-diff: the run in {args[sep + 1]} was interrupted')
    return 1 if new_failures or interrupted else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
