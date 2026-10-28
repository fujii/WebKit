#!/bin/bash
# Usage: measure.sh <build-dir> <tag> [reps]: M3 memory scenarios, results in out/<tag>-*.txt
set -u
D=$(dirname "$(readlink -f "$0")"); build=$1; tag=$2; reps=${3:-2}; out=$D/out; mkdir -p $out
port=18768; sock=wl-gpumem; rt=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}
python3 -I $D/chain-server.py $port & srv=$!
started=0
if ! pgrep -f "weston .*--socket=$sock( |$)" > /dev/null; then
    env -u DISPLAY -u WAYLAND_DISPLAY setsid nohup weston --backend=headless --renderer=gl --width=1920 --height=1080 --shell=kiosk --socket=$sock --no-config > $out/weston.log 2>&1 < /dev/null &
    started=1; for i in $(seq 1 50); do [ -S "$rt/$sock" ] && break; sleep 0.2; done
fi
export WAYLAND_DISPLAY=$sock
# mb <name> <url> <sample times (s since start)...>
mb() {
    local name=$1 url=$2; shift 2
    local f=$out/$tag-$name.txt t0=0
    env -u DISPLAY WEBKIT_DEBUG=ProcessSwapping WEBKIT_EXEC_PATH=$build/bin WEBKIT_INJECTED_BUNDLE_PATH=$build/lib LD_LIBRARY_PATH=$build/lib:${LD_LIBRARY_PATH:-} \
        setsid $build/bin/MiniBrowser --size=1920x1080 "$url" >> $out/$tag-$name.log 2>&1 & local pid=$!
    for t in "$@"; do sleep $((t - t0)); t0=$t; echo "t=$t $(python3 -I $D/procmem.py $pid)" >> $f; done
    kill $pid; sleep 2
}
for r in $(seq 1 $reps); do
    mb coop http://127.0.0.1:$port/coop 4 12
    mb chain http://127.0.0.1:$port/chain/1/5 4 14 24
    mb go204 http://127.0.0.1:$port/go204/2 6 15
    $D/nav-run.sh $build $tag
done
# WebDriver (no PSON: only the COOP response swaps processes), the M1A tool.
/home/fujii/webkit-wt/wpe-vibes/Tools/Scripts/webkit-gpu-memory --browser minibrowser --wpe --release --build-dir $build --size 1920x1080 \
    --repeat 3 --load-wait 3 --scroll-steps 0 --idle 10 --json $out/$tag-gm.json \
    http://127.0.0.1:$port/coop http://127.0.0.1:$port/plain > $out/$tag-gm.txt 2> $out/$tag-gm.log
kill $srv
[ $started = 1 ] && pkill -f "weston .*--socket=$sock( |$)"
exit 0
