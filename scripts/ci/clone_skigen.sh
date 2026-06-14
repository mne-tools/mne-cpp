#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#
# Clone Skigen at the centrally-configured git ref.
#
# The ref is resolved from src/external/external_deps.env:
#   --release  -> SKIGEN_RELEASE_REF (immutable tag, reproducible builds)
#   (default)  -> SKIGEN_DEV_REF     (moving development branch)
# An explicit --ref <ref> overrides both.

set -euo pipefail

usage()
{
    cat <<'EOF'
Usage: ./scripts/ci/clone_skigen.sh [--release] [--ref <ref>] [--output-dir <dir>] [--depth <n>]

  --release        Use SKIGEN_RELEASE_REF (default uses SKIGEN_DEV_REF).
  --ref <ref>      Explicit git ref (tag/branch) overriding the env defaults.
  --output-dir     Destination directory (default: src/external/skigen).
  --depth <n>      Shallow clone depth (default: 1; 0 disables --depth).
EOF
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${SCRIPT_DIR}/../../src/external/external_deps.env"

use_release=0
explicit_ref=""
output_dir="src/external/skigen"
depth=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release) use_release=1; shift ;;
        --ref) explicit_ref="$2"; shift 2 ;;
        --output-dir) output_dir="$2"; shift 2 ;;
        --depth) depth="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ -n "${explicit_ref}" ]]; then
    ref="${explicit_ref}"
elif [[ "${use_release}" -eq 1 ]]; then
    ref="${SKIGEN_RELEASE_REF}"
else
    ref="${SKIGEN_DEV_REF}"
fi

depth_arg=()
if [[ "${depth}" != "0" ]]; then
    depth_arg=(--depth "${depth}")
fi

echo "Cloning skigen (${ref}) into ${output_dir}..."
git clone "${depth_arg[@]}" --branch "${ref}" \
    https://github.com/skigen-project/skigen.git "${output_dir}"
