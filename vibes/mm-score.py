#!/usr/bin/env python3
# MotionMark result analysis for the WPE Vibes run. Agents must NOT edit this file.
#
# usage:
#   mm-score.py summary <result.json>...            per-test mean/median/sd/CV (+ overall geomean)
#   mm-score.py noise <result.json>... [-o FILE] [--max-cv X]
#       write per-test CV (default metrics/noise.json); exit 1 when a test's CV exceeds X (e.g. 0.03)
#   mm-score.py sanity <result.json>...             check the measurement environment of each run (exit 1 if wrong):
#       display refresh rate from the frame lengths at the lowest complexities ($MM_FPS_MIN-$MM_FPS_MAX, ~58.5 in
#       the headless Weston; a broken Weston gave 39), every score >= 10 (it gave 1.00 and 2.00), and from the .meta
#       of mm.sh that MiniBrowser was seen and loaded no external WPEPlatform module (wpe-platform-gtk was measured
#       by accident in runs 1-3)
#   mm-score.py compare --label LABEL [--target Suite/Test]...
#       compares metrics/mm/LABEL-A-*.json (before) with LABEL-B-*.json (after), see gate rules below.
#       exit 0 = accept, 1 = reject, 3 = borderline (rerun with twice the runs)
#   mm-score.py ci <baseline.json>... -- <final.json>...   bootstrap 95% CI of the relative change
#
# Gate rules ("Gates" in CLAUDE.md):
#   standard error of the difference: se = sqrt(sA^2/nA + sB^2/nB), where s = max(sample sd, CV_noise * mean)
#   target improved:   (meanB - meanA) >= 2 se  and  (meanB - meanA) / meanA >= 1%
#   other regressed:   (meanA - meanB) / meanA > 1%  and  (meanA - meanB) > 1.5 se
#   borderline:        exactly one of the two conditions of either rule holds
import glob
import json
import math
import os
import random
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.realpath(__file__))
STATE = subprocess.run(['bash', '-c', f'source {HERE}/limits.sh && echo "$VIBES_STATE"'], capture_output=True,
                       text=True).stdout.strip()
NOISE = os.path.join(STATE, 'metrics', 'noise.json')
MIN_SCORE = 10.0
FPS_MIN = float(os.environ.get('MM_FPS_MIN', 55))
FPS_MAX = float(os.environ.get('MM_FPS_MAX', 65))
MIN_GAIN = 0.01
GAIN_SE = 2.0
MAX_DROP = 0.01
DROP_SE = 1.5


def load(path):
    """Returns {"Suite/Test": [score per iteration]}."""
    with open(path) as f:
        data = json.load(f)
    out = {}
    for suite, body in data.items():
        if suite == 'debugOutput' or not isinstance(body, dict):
            continue
        suite_name = suite.split('-')[0]
        for test, t in body.get('tests', {}).items():
            score = t['metrics']['Score']
            values = score['current'] if isinstance(score, dict) else score
            out.setdefault(f'{suite_name}/{test.replace(" ", "")}', []).extend(float(v) for v in values)
    return out


def merge(paths):
    merged = {}
    iterations = []
    for p in paths:
        r = load(p)
        n = max((len(v) for v in r.values()), default=0)
        for i in range(n):
            iterations.append({k: v[i] for k, v in r.items() if i < len(v)})
        for k, v in r.items():
            merged.setdefault(k, []).extend(v)
    return merged, iterations


def overall(iterations):
    """Geometric mean of the MotionMark suite's 8 tests per iteration (only complete iterations)."""
    scores = []
    for it in iterations:
        vals = [v for k, v in it.items() if k.startswith('MotionMark/')]
        if len(vals) == 8:
            scores.append(math.exp(sum(math.log(v) for v in vals) / len(vals)))
    return scores


def stats(values):
    mean = statistics.fmean(values)
    sd = statistics.stdev(values) if len(values) > 1 else 0.0
    return {'n': len(values), 'mean': mean, 'median': statistics.median(values), 'sd': sd,
            'cv': sd / mean if mean else 0.0}


def cmd_summary(paths):
    merged, iterations = merge(paths)
    rows = dict(merged)
    o = overall(iterations)
    if o:
        rows['MotionMark (overall)'] = o
    print(f'{"test":45} {"n":>3} {"mean":>9} {"median":>9} {"sd":>8} {"cv%":>6}')
    for k, v in rows.items():
        s = stats(v)
        print(f'{k:45} {s["n"]:3} {s["mean"]:9.2f} {s["median"]:9.2f} {s["sd"]:8.2f} {100 * s["cv"]:6.2f}')
    return 0


def cmd_noise(args):
    out = NOISE
    max_cv = None
    if '-o' in args:
        i = args.index('-o')
        out = args[i + 1]
        args = args[:i] + args[i + 2:]
    if '--max-cv' in args:
        i = args.index('--max-cv')
        max_cv = float(args[i + 1])
        args = args[:i] + args[i + 2:]
    merged, iterations = merge(args)
    noise = {k: stats(v) for k, v in merged.items()}
    o = overall(iterations)
    if o:
        noise['MotionMark (overall)'] = stats(o)
    with open(out, 'w') as f:
        json.dump(noise, f, indent=1, sort_keys=True)
    print(f'wrote {out}')
    cmd_summary(args)
    if max_cv is not None:
        noisy = [k for k, s in noise.items() if s['cv'] > max_cv]
        if noisy:
            print(f'NOISY: CV > {100 * max_cv:.1f}%: {", ".join(noisy)}')
            return 1
    return 0


def display_fps(path):
    """Median frame rate of the frames at complexity <= 3 (the display refresh rate there), over all tests."""
    with open(path) as f:
        data = json.load(f)
    lengths = []
    for run in data.get('debugOutput', []):
        for iteration in run.get('data', []):
            for tests in iteration.values():
                for test in tests.values():
                    samples = test.get('samples', {}).get('controller', {})
                    fields = samples.get('fieldMap', {})
                    if not {'frameType', 'complexity', 'frameLength'} <= fields.keys():
                        continue
                    lengths += [row[fields['frameLength']] for row in samples.get('data', [])
                                if row[fields['complexity']] <= 3 and row[fields['frameType']] != 'm'][:60]
    return 1000 / statistics.median(lengths) if lengths else None


def cmd_sanity(paths):
    bad = 0
    for path in paths:
        problems = []
        fps = display_fps(path)
        if fps is None:
            problems.append('no frame samples')
        elif not FPS_MIN <= fps <= FPS_MAX:
            problems.append(f'display {fps:.1f} fps, expected {FPS_MIN:g}-{FPS_MAX:g}')
        low = {k: min(v) for k, v in load(path).items() if min(v) < MIN_SCORE}
        if low:
            problems.append('scores < 10: ' + ', '.join(f'{k} {v:.2f}' for k, v in low.items()))
        meta = {}
        try:
            with open(path + '.meta') as f:
                meta = dict(line.rstrip('\n').split('=', 1) for line in f if '=' in line)
        except OSError:
            pass
        if meta.get('minibrowser_seen') != 'yes':
            problems.append('MiniBrowser not seen by mm.sh (minibrowser_seen=' + meta.get('minibrowser_seen', 'missing') + ')')
        if meta.get('platform_modules') != 'none':
            problems.append('external WPEPlatform module loaded: ' + meta.get('platform_modules', 'not recorded'))
        name = os.path.basename(path)
        if problems:
            print(f'SANITY FAIL {name}: ' + '; '.join(problems))
            bad = 1
        else:
            print(f'sanity ok   {name}: {fps:.1f} fps')
    return bad


def cmd_compare(args):
    label, targets = None, []
    while args:
        if args[0] == '--label':
            label, args = args[1], args[2:]
        elif args[0] == '--target':
            targets.append(args[1])
            args = args[2:]
        else:
            print(f'unknown argument {args[0]}')
            return 2
    mm = os.path.join(STATE, 'metrics', 'mm')
    a_paths = sorted(glob.glob(os.path.join(mm, f'{label}-A-*.json')))
    b_paths = sorted(glob.glob(os.path.join(mm, f'{label}-B-*.json')))
    if not a_paths or not b_paths:
        print(f'no results for label {label}')
        return 2
    a, a_it = merge(a_paths)
    b, b_it = merge(b_paths)
    oa, ob = overall(a_it), overall(b_it)
    if oa and ob:
        a['MotionMark (overall)'], b['MotionMark (overall)'] = oa, ob
    noise = {}
    if os.path.exists(NOISE):
        with open(NOISE) as f:
            noise = json.load(f)
    if not targets:
        print('warning: no --target given; checking regressions only')

    verdict = 0
    print(f'{"test":40} {"nA":>3} {"nB":>3} {"meanA":>9} {"meanB":>9} {"change%":>8} {"diff/se":>8}  verdict')
    for test in sorted(set(a) & set(b)):
        sa, sb = stats(a[test]), stats(b[test])
        cv = noise.get(test, {}).get('cv', 0.0)
        s_a = max(sa['sd'], cv * sa['mean'])
        s_b = max(sb['sd'], cv * sb['mean'])
        se = math.sqrt(s_a ** 2 / sa['n'] + s_b ** 2 / sb['n'])
        diff = sb['mean'] - sa['mean']
        rel = diff / sa['mean']
        z = diff / se if se else (0.0 if diff == 0 else math.copysign(float('inf'), diff))
        if test in targets:
            c1, c2 = z >= GAIN_SE, rel >= MIN_GAIN
            v = 'IMPROVED' if c1 and c2 else ('BORDERLINE' if c1 or c2 else 'NOT IMPROVED')
            if v == 'NOT IMPROVED':
                verdict = max(verdict, 1)
            elif v == 'BORDERLINE':
                verdict = 3 if verdict != 1 else 1
        else:
            c1, c2 = -rel > MAX_DROP, -z > DROP_SE
            v = 'REGRESSED' if c1 and c2 else ('BORDERLINE' if c1 or c2 else 'ok')
            if v == 'REGRESSED':
                verdict = 1
            elif v == 'BORDERLINE' and verdict == 0:
                verdict = 3
        print(f'{test:40} {sa["n"]:3} {sb["n"]:3} {sa["mean"]:9.2f} {sb["mean"]:9.2f} {100 * rel:8.2f} {z:8.2f}  {v}')
    for t in targets:
        if t not in a or t not in b:
            print(f'target {t} has no results')
            verdict = 1
    print({0: 'ACCEPT', 1: 'REJECT', 3: 'BORDERLINE: rerun with twice the runs'}[verdict])
    return verdict


def bootstrap_ci(xa, xb, rounds=10000, seed=1):
    rng = random.Random(seed)
    changes = []
    for _ in range(rounds):
        ma = statistics.fmean(rng.choices(xa, k=len(xa)))
        mb = statistics.fmean(rng.choices(xb, k=len(xb)))
        changes.append(mb / ma - 1)
    changes.sort()
    return changes[int(0.025 * rounds)], changes[int(0.975 * rounds)]


def cmd_ci(args):
    sep = args.index('--')
    a, a_it = merge(args[:sep])
    b, b_it = merge(args[sep + 1:])
    oa, ob = overall(a_it), overall(b_it)
    if oa and ob:
        a['MotionMark (overall)'], b['MotionMark (overall)'] = oa, ob
    print(f'{"test":40} {"meanA":>9} {"meanB":>9} {"change%":>8} {"95% CI":>18}')
    for test in sorted(set(a) & set(b)):
        ma, mb = statistics.fmean(a[test]), statistics.fmean(b[test])
        lo, hi = bootstrap_ci(a[test], b[test])
        print(f'{test:40} {ma:9.2f} {mb:9.2f} {100 * (mb / ma - 1):8.2f} [{100 * lo:6.2f}, {100 * hi:6.2f}]')
    return 0


def main(argv):
    if len(argv) < 2:
        print(open(__file__).read().split('\nimport')[0])
        return 2
    cmd, args = argv[1], argv[2:]
    return {'summary': cmd_summary, 'noise': cmd_noise, 'compare': cmd_compare, 'ci': cmd_ci, 'sanity': cmd_sanity}[cmd](args)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
