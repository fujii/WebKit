#!/bin/bash
# M3 measurement chain: base (main build), +m3a, +m3a+m3b (w1 builds). Each step through heavy.sh.
set -u
D=$(dirname "$(readlink -f "$0")"); M=/home/fujii/webkit-wt/wpe-vibes; W=/home/fujii/webkit-wt/wpe-vibes-w1
log() { echo "$(date -Iseconds) $*" >> $D/out/chain.log; }
mkdir -p $D/out
run() { local name=$1; shift; $M/vibes/heavy.sh "$name" "$@"; local rc=$?; log "$name rc=$rc"; return $rc; }
run m3-measure-base $D/measure.sh $M/WebKitBuild/WPE/Release base || exit 1
git -C $W switch --detach task-m3~1 >> $D/out/chain.log 2>&1 || exit 1
run build-w1 $M/vibes/build.sh $W || exit 1
run m3-measure-m3a $D/measure.sh $W/WebKitBuild/WPE/Release m3a || exit 1
git -C $W switch task-m3 >> $D/out/chain.log 2>&1 || exit 1
run build-w1 $M/vibes/build.sh $W || exit 1
run m3-measure-m3ab $D/measure.sh $W/WebKitBuild/WPE/Release m3ab || exit 1
log done
