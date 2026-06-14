#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#
# Load external dependency versions from src/external/external_deps.env and
# export them to the GitHub Actions environment ($GITHUB_ENV) so downstream
# steps can reference $EIGEN_VERSION, $ONNXRUNTIME_VERSION, $SKIGEN_RELEASE_REF
# and $SKIGEN_DEV_REF. When run outside Actions it just prints the values.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck disable=SC1091
source "${SCRIPT_DIR}/../../src/external/external_deps.env"

emit()
{
    local name="$1"
    local value="$2"
    echo "${name}=${value}"
    if [[ -n "${GITHUB_ENV:-}" ]]; then
        printf '%s=%s\n' "${name}" "${value}" >> "${GITHUB_ENV}"
    fi
}

emit EIGEN_VERSION        "${EIGEN_VERSION}"
emit ONNXRUNTIME_VERSION  "${ONNXRUNTIME_VERSION}"
emit SKIGEN_RELEASE_REF   "${SKIGEN_RELEASE_REF}"
emit SKIGEN_DEV_REF       "${SKIGEN_DEV_REF}"
