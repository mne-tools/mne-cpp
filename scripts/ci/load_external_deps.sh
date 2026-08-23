#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#
# Load external dependency versions from src/external/external_deps.env and
# export them to the GitHub Actions environment ($GITHUB_ENV) so downstream
# steps can reference the shared dependency versions. When run outside Actions
# it just prints the values. Pass "min" to select QT_MIN_VERSION for builds;
# the default "max" selects QT_VERSION.

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
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
        printf '%s=%s\n' "${name}" "${value}" >> "${GITHUB_OUTPUT}"
    fi
}

case "${1:-max}" in
    min) QT_BUILD_VERSION="${QT_MIN_VERSION}" ;;
    max) QT_BUILD_VERSION="${QT_VERSION}" ;;
    *) echo "Expected Qt version selector 'min' or 'max', got: $1" >&2; exit 2 ;;
esac

emit QT_MIN_VERSION       "${QT_MIN_VERSION}"
emit QT_VERSION           "${QT_VERSION}"
emit QT_VERSION_TOKEN     "${QT_VERSION//./}"
emit QT_BUILD_VERSION     "${QT_BUILD_VERSION}"
emit QT_VERSION_MATRIX    "{\"include\":[{\"qt_version\":\"${QT_MIN_VERSION}\",\"version_token\":\"${QT_MIN_VERSION//./}\"},{\"qt_version\":\"${QT_VERSION}\",\"version_token\":\"${QT_VERSION//./}\"}]}"
emit EIGEN_VERSION        "${EIGEN_VERSION}"
emit ONNXRUNTIME_VERSION  "${ONNXRUNTIME_VERSION}"
emit SKIGEN_RELEASE_REF   "${SKIGEN_RELEASE_REF}"
emit SKIGEN_DEV_REF       "${SKIGEN_DEV_REF}"
