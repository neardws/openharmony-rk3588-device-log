#!/system/bin/sh

# hcodec_minidec surface/buffer validation suite for OpenHarmony devices.
#
# Usage:
#   /data/local/tmp/hcodec_minidec_suite.sh <input.h264> <width> <height> [surface|buffer|buffer-sync|buffer-async|all]
#
# Default mode: all
# Output logs: /data/local/tmp/hcodec_logs/<run_id>/

set -u

if [ "$#" -lt 3 ] || [ "$#" -gt 4 ]; then
    echo "usage: $0 <input.h264> <width> <height> [surface|buffer|buffer-sync|buffer-async|all]" >&2
    exit 2
fi

INPUT="$1"
WIDTH="$2"
HEIGHT="$3"
MODE="${4:-all}"

BIN="${HCODEC_MINIDEC_BIN:-/data/local/tmp/hcodec_minidec}"
LOG_ROOT="${HCODEC_LOG_ROOT:-/data/local/tmp/hcodec_logs}"
RUN_ID="${HCODEC_RUN_ID:-}"

if [ -z "$RUN_ID" ]; then
    RUN_ID="$(date +%Y%m%d_%H%M%S 2>/dev/null)"
fi
if [ -z "$RUN_ID" ]; then
    RUN_ID="run_$$"
fi

OUT_DIR="$LOG_ROOT/$RUN_ID"
MASTER_LOG="$OUT_DIR/suite.log"
RESULTS_FILE="$OUT_DIR/results.txt"
SUMMARY_FILE="$OUT_DIR/summary.txt"

mkdir -p "$OUT_DIR" || exit 1
: > "$MASTER_LOG"
: > "$RESULTS_FILE"
: > "$SUMMARY_FILE"

log()
{
    echo "[suite] $*"
    echo "[suite] $*" >> "$MASTER_LOG"
}

capture_cmd()
{
    name="$1"
    shift
    "$@" > "$OUT_DIR/${name}.txt" 2>&1
}

capture_snapshot()
{
    tag="$1"
    capture_cmd "hidumper_3002_${tag}" hidumper -s 3002
    capture_cmd "hidumper_3011_${tag}" hidumper -s 3011
    capture_cmd "ps_${tag}" ps -ef
}

append_summary_from_log()
{
    log_file="$1"
    case_name="$2"
    {
        echo "== ${case_name} =="
        grep -E '^\[summary\]|^\[codec-error\]|^\[error\]|^\[surface\]|^\[wait-' "$log_file" 2>/dev/null
        echo
    } >> "$SUMMARY_FILE"
}

run_case()
{
    case_name="$1"
    output_mode="$2"
    pixel_format="$3"
    drain_mode="$4"
    log_file="$OUT_DIR/${case_name}.log"

    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/system/lib:/system/lib/platformsdk:/system/lib/chipset-pub-sdk:/data/local/tmp/hcodec_lib}"
    export HCODEC_MINIDEC_FRAME_RATE="${HCODEC_MINIDEC_FRAME_RATE:-60}"
    export HCODEC_MINIDEC_PROGRESS_MS="${HCODEC_MINIDEC_PROGRESS_MS:-1000}"
    export HCODEC_MINIDEC_LOG_INTERVAL="${HCODEC_MINIDEC_LOG_INTERVAL:-0}"
    export HCODEC_MINIDEC_PIXEL_FORMAT="$pixel_format"
    export HCODEC_MINIDEC_DRAIN="$drain_mode"

    if [ "$output_mode" = "surface" ]; then
        export HCODEC_MINIDEC_OUTPUT=surface
    else
        unset HCODEC_MINIDEC_OUTPUT
    fi

    log "run case=${case_name} output=${output_mode} pixel=${pixel_format} drain=${drain_mode}"
    (
        echo "[case] name=${case_name}"
        echo "[case] bin=${BIN}"
        echo "[case] input=${INPUT} width=${WIDTH} height=${HEIGHT}"
        echo "[case] LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
        echo "[case] HCODEC_MINIDEC_OUTPUT=${HCODEC_MINIDEC_OUTPUT:-buffer}"
        echo "[case] HCODEC_MINIDEC_PIXEL_FORMAT=${HCODEC_MINIDEC_PIXEL_FORMAT}"
        echo "[case] HCODEC_MINIDEC_DRAIN=${HCODEC_MINIDEC_DRAIN}"
        echo "[case] HCODEC_MINIDEC_FRAME_RATE=${HCODEC_MINIDEC_FRAME_RATE}"
        echo "[case] HCODEC_MINIDEC_PROGRESS_MS=${HCODEC_MINIDEC_PROGRESS_MS}"
        echo "[case] HCODEC_MINIDEC_LOG_INTERVAL=${HCODEC_MINIDEC_LOG_INTERVAL}"
        "$BIN" "$INPUT" "$WIDTH" "$HEIGHT"
        ret="$?"
        echo "[case] exit_code=${ret}"
        exit "$ret"
    ) > "$log_file" 2>&1
    ret="$?"

    echo "${case_name} ${ret}" >> "$RESULTS_FILE"
    append_summary_from_log "$log_file" "$case_name"
    capture_snapshot "$case_name"
    log "done case=${case_name} exit=${ret}"
    return 0
}

if [ ! -x "$BIN" ]; then
    log "binary not executable: $BIN"
    exit 3
fi

if [ ! -f "$INPUT" ]; then
    log "input not found: $INPUT"
    exit 4
fi

log "run_id=${RUN_ID} out_dir=${OUT_DIR} mode=${MODE}"
capture_cmd env_uname uname -a
capture_cmd env_getconf getconf LONG_BIT
capture_cmd env_ls_bin ls -l "$BIN"
capture_cmd env_ls_input ls -l "$INPUT"
capture_snapshot before

FAILURES=0

case "$MODE" in
    surface)
        run_case surface_minimal surface surface async
        ;;
    buffer)
        run_case buffer_sync buffer nv12 sync
        run_case buffer_async buffer nv12 async
        ;;
    buffer-sync)
        run_case buffer_sync buffer nv12 sync
        ;;
    buffer-async)
        run_case buffer_async buffer nv12 async
        ;;
    all)
        run_case surface_minimal surface surface async
        run_case buffer_sync buffer nv12 sync
        run_case buffer_async buffer nv12 async
        ;;
    *)
        log "unknown mode: $MODE"
        exit 5
        ;;
esac

capture_snapshot after

while read -r name code; do
    if [ "$code" != "0" ]; then
        FAILURES=$((FAILURES + 1))
    fi
done < "$RESULTS_FILE"

log "results=$(tr '\n' ';' < "$RESULTS_FILE" 2>/dev/null)"
log "summary_file=${SUMMARY_FILE}"
log "failures=${FAILURES}"

if [ "$FAILURES" -eq 0 ]; then
    exit 0
fi
exit 10
