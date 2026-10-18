#!/bin/sh
# Runs every browser through the site list inside the headless Weston at $WAYLAND_DISPLAY.
# Usage: gpu-memory/run-compare.sh <tag> [repeat] [browser...]
set -u
cd "$(dirname "$0")/.."
tag=$1; repeat=${2:-2}; shift 2 || shift $#
browsers=${*:-"minibrowser chrome firefox"}
# MiniBrowser would otherwise use wpe-platform-gtk from /usr/local (higher priority than the built-in Wayland).
export WPE_PLATFORM=${WPE_PLATFORM:-wayland}
mkdir -p gpu-memory/results
for browser in $browsers; do
    extra=""
    [ "$browser" = minibrowser ] && extra="--wpe --release ${BUILD_DIR:+--build-dir $BUILD_DIR}"
    [ "$browser" = chrome ] && extra="--profile $HOME/.cache/webkit-gpu-memory/chrome"
    WEBKIT_GPU_MEMORY_DEBUG=1 Tools/Scripts/webkit-gpu-memory --browser "$browser" $extra --size 1920x1080 --repeat "$repeat" \
        --urls-file gpu-memory/sites.txt --json "gpu-memory/results/$tag-$browser.json" \
        > "gpu-memory/results/$tag-$browser.txt" 2> "gpu-memory/results/$tag-$browser.log"
done
