#!/bin/bash
# WPE Vibes driver (run 2+): one fresh `claude -p` orchestrator session per iteration until the deadline.
# usage: nohup setsid vibes/loop.sh <hours>|<HH:MM> > vibes/logs/loop.log 2>&1 &
# Stop early: touch vibes/run/stop (the current iteration finishes first).
# It pins the gate scripts and agent rules the gates trust (commit changes to them before starting it).
# Per-iteration output: logs/iter-NNN.json; token usage: metrics/usage.tsv.
set -u
here=$(dirname "$(readlink -f "$0")")
source "$here/limits.sh"
source "$here/gate-lib.sh"
V=$VIBES_STATE
[ $# -eq 1 ] || { sed -n '2,6p' "$0"; exit 2; }
cd "$VIBES_MAIN"
pin_tools || exit 1
plugin=()
[ -n "$VIBES_PLUGIN_DIR" ] && plugin=(--plugin-dir "$VIBES_PLUGIN_DIR")
if [[ $1 == *:* ]]; then   # absolute deadline HH:MM (next occurrence)
    deadline=$(date -d "$1" +%s); [ $deadline -le $(date +%s) ] && deadline=$((deadline + 86400))
else
    deadline=$(( $(date +%s) + $1 * 3600 ))
fi
echo "- $(date -Is) loop start, deadline $(date -d @$deadline -Is)" >> $V/LOG.md
usage=$V/metrics/usage.tsv
[ -f $usage ] || printf 'iter\tstart\tsec\trc\terror\tcost_usd\tturns\tmodel:in/out/cache_read/cache_write/cost\tresult\n' > $usage
i=$(ls $V/logs/iter-*.json 2>/dev/null | wc -l); fails=0; idles=0
while [ $(date +%s) -lt $((deadline - 900)) ] && [ ! -f "$VIBES_STOP" ]; do
    i=$((i + 1)); n=$(printf %03d $i); rm -f $V/next-action
    start=$(date +%s)
    prompt=$(sed -e "s/{DEADLINE}/$(date -d @$deadline -Is)/g" -e "s/{NOW}/$(date -Is)/g" $V/ITERATION.md)
    limit=$(( deadline + 1800 - start )); [ $limit -gt 10800 ] && limit=10800
    timeout $limit claude -p "$prompt" --model opus --effort medium --permission-mode auto --permission-prompts none \
        "${plugin[@]}" --output-format json > $V/logs/iter-$n.json 2> $V/logs/iter-$n.err
    rc=$?
    line=$(python3 - $V/logs/iter-$n.json <<'PY'
import json, sys
try:
    d = json.load(open(sys.argv[1]))
except Exception as e:
    print(f"\t\t\tno json: {e}"); sys.exit()
m = ';'.join(f"{k.replace('claude-', '')}:{u.get('inputTokens', 0)}/{u.get('outputTokens', 0)}/{u.get('cacheReadInputTokens', 0)}/{u.get('cacheCreationInputTokens', 0)}/{u.get('costUSD', 0):.2f}" for k, u in d.get('modelUsage', {}).items())
r = (d.get('result') or '').strip().splitlines()
print(f"{d.get('is_error')}\t{d.get('total_cost_usd', 0):.2f}\t{d.get('num_turns')}\t{m}\t{r[-1][:200] if r else ''}")
PY
)
    printf '%s\t%s\t%s\t%s\t%s\n' $n "$(date -d @$start -Is)" $(( $(date +%s) - start )) $rc "$line" >> $usage
    echo "$(date -Is) iter $n rc=$rc $line"
    if [ $rc -ne 0 ] || [[ "$line" == True* ]]; then
        # Claude plan limit ("You've hit your session limit · resets 10:20pm (Asia/Tokyo)"): sleep until the reset
        # instead of counting a failure. Other limits/overload: 15 min, also not counted.
        reset=$(grep -ohiE "hit your [a-z ]*limit[^\"]*resets [0-9:apm]+" $V/logs/iter-$n.json $V/logs/iter-$n.err 2>/dev/null | head -1 | grep -oE '[0-9:]+ ?[apm]+$')
        if [ -n "$reset" ]; then
            until_s=$(date -d "$reset" +%s 2>/dev/null || echo 0)
            [ $until_s -le $(date +%s) ] && until_s=$((until_s + 86400))
            wait_s=$((until_s - $(date +%s) + 120)); [ $wait_s -gt 21600 ] && wait_s=21600
            echo "- $(date -Is) loop: Claude limit hit; sleeping until $(date -d @$((until_s + 120)) -Is)" >> $V/LOG.md
            sleep $wait_s; continue
        fi
        if grep -qiE 'usage limit|rate limit|429|overloaded' $V/logs/iter-$n.json $V/logs/iter-$n.err 2>/dev/null; then
            echo "$(date -Is) limit hit; sleeping 15 min"; sleep 900; continue
        fi
        fails=$((fails + 1))
        [ $fails -ge 4 ] && { echo "- $(date -Is) loop: stopped after 4 failed iterations" >> $V/LOG.md; exit 1; }
        continue
    fi
    fails=0
    action=$(cat $V/next-action 2>/dev/null)
    [ "$action" = idle ] && idles=$((idles + 1)) || idles=0
    if [ "$action" = wait-gate ]; then
        echo "$(date -Is) waiting for the gate"
        w=0
        while pgrep -f 'gate-run.sh' > /dev/null && [ $w -lt 7200 ] && [ $(date +%s) -lt $deadline ]; do sleep 60; w=$((w + 60)); done
    elif [ "$action" = idle ]; then
        # Nothing to do: do not start the next session right away (empty iterations cost tokens and hit the plan
        # limit). Stop after 3 idle iterations in a row.
        [ $idles -ge 3 ] && { echo "- $(date -Is) loop: stopped after $idles idle iterations" >> $V/LOG.md; break; }
        echo "$(date -Is) idle ($idles); sleeping 15 min"
        sleep 900
    fi
done
echo "- $(date -Is) loop end ($i iterations)" >> $V/LOG.md
