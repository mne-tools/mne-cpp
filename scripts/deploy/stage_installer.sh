#!/bin/bash
#
# Prepare the online-installer staging layout without invoking binarycreator or
# repogen. This is mainly useful for debugging the Qt Installer Framework
# package tree locally.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OUTPUT_DIR="${REPO_ROOT}/build_installer/staging"
RELEASE_TAG="${1:-dev_build}"
PLATFORM="${2:-linux}"

ASSET_PREFIX=""
case "${RELEASE_TAG}" in
    dev_build)
        ASSET_PREFIX="mne-cpp-dev"
        ;;
    latest)
        ASSET_PREFIX="mne-cpp-latest"
        ;;
    *)
        ASSET_PREFIX="mne-cpp-${RELEASE_TAG}"
        ;;
esac

"${SCRIPT_DIR}/prepare_bootstrap_installer.sh" \
    --release-tag "${RELEASE_TAG}" \
    --asset-prefix "${ASSET_PREFIX}" \
    --platform "${PLATFORM}" \
    --repository-url "file://${REPO_ROOT}/build_installer/repository" \
    --output-dir "${OUTPUT_DIR}"

echo ""
echo "Online installer staging prepared at:"
echo "  ${OUTPUT_DIR}"
