#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Measures the benchmarks and regenerates the website's Performance page from them.
#   scripts/bench/refresh.sh [--build <dir>] [run_benchmarks.py options]
# Run it on a quiet machine: other load shows up in the numbers.
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
results="$root/doc/website/static/bench/results.json"
python3 "$root/scripts/bench/run_benchmarks.py" --out "$results" "$@"
python3 "$root/scripts/bench/render_charts.py" "$results" "$root/doc/website/static/bench" \
    --page "$root/doc/website/docs/performance.mdx"
