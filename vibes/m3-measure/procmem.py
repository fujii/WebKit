# Usage: procmem.py <ancestor-pid>: GPU (DRM fdinfo) and PSS of the WPEWebProcesses under <ancestor-pid>, one line.
import glob, os, subprocess, sys
root = sys.argv[1]
def ancestors(p):
    for _ in range(8):
        try: p = open(f'/proc/{p}/stat').read().rsplit(')', 1)[1].split()[1]
        except OSError: return
        yield p
procs = []
for pid in subprocess.run(['pgrep', '-x', 'WPEWebProcess'], capture_output=True, text=True).stdout.split():
    if root not in ancestors(pid): continue
    clients = {}
    for f in glob.glob(f'/proc/{pid}/fdinfo/*'):
        try: t = open(f).read()
        except OSError: continue
        if 'drm-client-id' not in t: continue
        d = dict(l.split(':', 1) for l in t.splitlines() if ':' in l)
        keys = [k for k in d if k.startswith('drm-total-')] or [k for k in d if k.startswith('drm-memory-')]
        clients[d['drm-client-id'].strip()] = sum(int(d[k].split()[0]) for k in keys if not k.endswith('cpu'))
    try: pss = int([l for l in open(f'/proc/{pid}/smaps_rollup') if l.startswith('Pss:')][0].split()[1])
    except (OSError, IndexError): pss = 0
    procs.append((pid, sum(clients.values()) / 1024, pss / 1024))
print(f"procs={len(procs)} gpu={sum(p[1] for p in procs):.1f} pss={sum(p[2] for p in procs):.1f} | " + ' '.join(f"{p[0]}:{p[1]:.1f}/{p[2]:.1f}" for p in procs))
