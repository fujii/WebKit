#!/bin/bash
# Usage: run.sh <build-dir> <tag>: A -> B (cross-site, process swap) -> back; GPU per WebProcess while A is suspended.
build=$1; tag=$2; port=18767; D=$(dirname "$(readlink -f "$0")"); mkdir -p $D/out; out=$D/out/$tag-nav.txt
python3 -I $D/nav-server.py $port > $D/out/$tag-nav-server.txt 2>&1 & srv=$!
echo "== run" >> $out
pgrep -f "weston .*--socket=wl-gpumem( |$)" >/dev/null || { env -u DISPLAY -u WAYLAND_DISPLAY setsid nohup weston --backend=headless --renderer=gl --width=1920 --height=1080 --shell=kiosk --socket=wl-gpumem --no-config >$D/out/weston.log 2>&1 </dev/null & sleep 2; }
env -u DISPLAY WAYLAND_DISPLAY=wl-gpumem WEBKIT_EXEC_PATH=$build/bin WEBKIT_INJECTED_BUNDLE_PATH=$build/lib LD_LIBRARY_PATH=$build/lib:${LD_LIBRARY_PATH:-} \
    setsid $build/bin/MiniBrowser --size=1920x1080 http://127.0.0.1:$port/a > $D/out/$tag-nav.log 2>&1 & mb=$!
gpu() {
    for p in $(pgrep -x WPEWebProcess); do
        q=$p; ok=0; for i in 1 2 3 4 5 6 7 8; do q=$(awk '{print $4}' /proc/$q/stat 2>/dev/null); [ -z "$q" ] && break; [ "$q" = "$mb" ] && { ok=1; break; }; done
        [ $ok = 1 ] || continue
        python3 -I - $p <<'PY'
import glob, sys
pid = sys.argv[1]; clients = {}
for f in glob.glob(f'/proc/{pid}/fdinfo/*'):
    try: t = open(f).read()
    except OSError: continue
    if 'drm-client-id' not in t: continue
    d = dict(l.split(':', 1) for l in t.splitlines() if ':' in l)
    keys = [k for k in d if k.startswith('drm-total-')] or [k for k in d if k.startswith('drm-memory-')]
    clients[d['drm-client-id'].strip()] = (sum(int(d[k].split()[0]) for k in keys if not k.endswith('cpu')), sum(int(d[k].split()[0]) for k in d if k.startswith('drm-shared-') and not k.endswith('cpu')))
if clients: print(f"pid {pid}: {sum(c[0] for c in clients.values())/1024:.1f} MiB (shared {sum(c[1] for c in clients.values())/1024:.1f})")
PY
    done
}
sleep 2; echo "A shown:" >> $out; gpu >> $out
sleep 5; echo "B shown, A suspended:" >> $out; gpu >> $out
sleep 6; echo "back to A:" >> $out; gpu >> $out
kill $mb; sleep 1; kill $srv
