#!/usr/bin/env python3
# Memory watchdog for the WPE Vibes run. Polls every second and SIGKILLs the running
# heavy job (see heavy.sh) when memory gets tight. Never uses SIGSTOP.
# usage: memwatch.py  (run it with nohup/setsid; it exits if another instance is running)
# Its pid is in $VIBES_MEMWATCH_PID; heavy.sh refuses to start a job without it.
import os
import re
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.realpath(__file__))


def load_limits():
    out = subprocess.run(['bash', '-c', f'source {HERE}/limits.sh; set'], capture_output=True, text=True).stdout
    limits = {}
    for line in out.splitlines():
        m = re.match(r'^(VIBES_\w+|MEM_\w+|PSI_\w+|SWAPIN_\w+)=(.*)$', line)
        if m:
            limits[m.group(1)] = m.group(2).strip("'")
    return limits


L = load_limits()
MEM_KILL_KB = int(L['MEM_KILL_GB']) * 1024 * 1024
PSI_KILL = float(L['PSI_FULL_AVG10_KILL'])
SWAPIN_RATE = int(L['SWAPIN_PAGES_PER_SEC'])
SWAPIN_SECONDS = int(L['SWAPIN_SECONDS'])
JOB = L['VIBES_JOB']
MEMKILL = L['VIBES_MEMKILL']
PIDFILE = L['VIBES_MEMWATCH_PID']
LOG = os.path.join(L['VIBES_STATE'], 'LOG.md')


def mem_available_kb():
    with open('/proc/meminfo') as f:
        for line in f:
            if line.startswith('MemAvailable:'):
                return int(line.split()[1])
    return 0


def psi_full_avg10():
    """0 when the kernel has no PSI (e.g. psi=0), so only the other checks apply."""
    try:
        with open('/proc/pressure/memory') as f:
            for line in f:
                if line.startswith('full'):
                    return float(line.split()[1].split('=')[1])
    except OSError:
        pass
    return 0.0


def is_memwatch(pid):
    try:
        with open(f'/proc/{pid}/cmdline', 'rb') as f:
            return b'memwatch.py' in f.read()
    except OSError:
        return False


def pswpin():
    with open('/proc/vmstat') as f:
        for line in f:
            if line.startswith('pswpin '):
                return int(line.split()[1])
    return 0


def job_processes(sid, job_id):
    """Processes of the job: same session, descendants of the session leader, or tagged by env."""
    procs = {}
    for pid in os.listdir('/proc'):
        if not pid.isdigit():
            continue
        try:
            with open(f'/proc/{pid}/stat') as f:
                stat = f.read()
            fields = stat[stat.rfind(')') + 2:].split()
            procs[int(pid)] = (int(fields[1]), int(fields[3]))  # ppid, session
        except OSError:
            pass
    tag = f'VIBES_HEAVY_ID={job_id}'.encode()

    def tagged(p):
        try:
            with open(f'/proc/{p}/environ', 'rb') as f:
                return tag in f.read().split(b'\0')
        except OSError:
            return False

    victims = set()
    # A stale job file (heavy.sh itself killed) may name a session id that is now reused by an unrelated process:
    # use the session and the process tree only while the session leader is still the tagged job.
    if tagged(sid):
        victims = {p for p, (_, s) in procs.items() if s == sid}
        victims.add(sid)
        changed = True
        while changed:
            changed = False
            for p, (ppid, _) in procs.items():
                if ppid in victims and p not in victims:
                    victims.add(p)
                    changed = True
    victims |= {p for p in procs if p not in victims and tagged(p)}
    victims.discard(os.getpid())
    return victims


def kill_job(reason):
    try:
        with open(JOB) as f:
            sid_s, job_id, name = f.read().split(maxsplit=2)
    except (OSError, ValueError):
        return False
    sid = int(sid_s)
    victims = job_processes(sid, job_id)
    for p in victims:
        try:
            os.kill(p, signal.SIGKILL)
        except OSError:
            pass
    stamp = time.strftime('%Y-%m-%dT%H:%M:%S%z')
    msg = f'{stamp} memwatch killed {job_id} {name.strip()} ({len(victims)} processes): {reason}'
    with open(MEMKILL, 'w') as f:
        f.write(msg + '\n')
    with open(LOG, 'a') as f:
        f.write(f'- {msg}\n')
    print(msg, flush=True)
    return True


def main():
    try:
        with open(PIDFILE) as f:
            other = int(f.read().strip() or 0)
    except (OSError, ValueError):
        other = 0
    if other and other != os.getpid() and is_memwatch(other):
        print(f'memwatch.py is already running (pid {other})', file=sys.stderr)
        return 1
    with open(PIDFILE, 'w') as f:
        f.write(f'{os.getpid()}\n')
    if not os.path.exists('/proc/pressure/memory'):
        print('memwatch: no /proc/pressure/memory; the PSI check is off', flush=True)
    last_swpin = pswpin()
    swap_streak = 0
    while True:
        time.sleep(1)
        avail = mem_available_kb()
        psi = psi_full_avg10()
        cur = pswpin()
        rate = cur - last_swpin
        last_swpin = cur
        swap_streak = swap_streak + 1 if rate > SWAPIN_RATE else 0
        reason = None
        if avail < MEM_KILL_KB:
            reason = f'MemAvailable {avail // 1048576}GB < {MEM_KILL_KB // 1048576}GB'
        elif psi > PSI_KILL:
            reason = f'PSI full avg10 {psi} > {PSI_KILL}'
        elif swap_streak >= SWAPIN_SECONDS:
            reason = f'pswpin {rate}/s for {swap_streak}s'
        if reason and os.path.exists(JOB):
            if kill_job(reason):
                swap_streak = 0
                time.sleep(5)


if __name__ == '__main__':
    sys.exit(main())
