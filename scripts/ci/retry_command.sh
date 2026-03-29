#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "usage: $0 <attempts> <sleep-seconds> <command> [args...]" >&2
    exit 1
fi

attempts="$1"
sleep_seconds="$2"
shift 2

for ((attempt = 1; attempt <= attempts; attempt++)); do
    if "$@"; then
        exit 0
    fi

    if (( attempt == attempts )); then
        echo "command failed after ${attempts} attempts: $*" >&2
        exit 1
    fi

    echo "attempt ${attempt}/${attempts} failed for: $*" >&2
    echo "retrying in ${sleep_seconds}s..." >&2
    sleep "${sleep_seconds}"
done
