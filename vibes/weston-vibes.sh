#!/bin/bash
# Ensure the headless Weston used for MotionMark is running (1920x1080, 60 Hz nominal, kiosk shell, GL renderer).
# Prints the socket name. MotionMark runs there instead of on the host display, so the host display may be off.
# weston-vibes.ini sets repaint-window=16: the default gives 39 fps on the headless backend (see the .ini).
set -u
sock=wl-vibes
source "$(dirname "$(readlink -f "$0")")/limits.sh"
ini=$VIBES_STATE/weston-vibes.ini
rt=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}
if ! pgrep -f "weston .*--socket=$sock --config=$ini" > /dev/null; then
    pgrep -f "weston .*--socket=$sock( |$)" | xargs -r kill
    for i in $(seq 1 25); do pgrep -f "weston .*--socket=$sock( |$)" > /dev/null || break; sleep 0.2; done
    # A killed Weston leaves its socket behind; without removing it the wait below would not wait.
    rm -f "$rt/$sock" "$rt/$sock.lock"
    log=$VIBES_STATE/logs/weston-vibes.log
    env -u VIBES_HEAVY_ID -u DISPLAY -u WAYLAND_DISPLAY setsid nohup weston --backend=headless --renderer=gl \
        --width=1920 --height=1080 --refresh-rate=60000 --shell=kiosk --socket=$sock --config=$ini \
        >> "$log" 2>&1 < /dev/null &
    for i in $(seq 1 50); do [ -S "$rt/$sock" ] && break; sleep 0.2; done
    [ -S "$rt/$sock" ] || { echo "weston-vibes.sh: Weston did not start, see $log" >&2; exit 1; }
fi
echo $sock
