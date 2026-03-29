#!/bin/bash
#
# Build a lightweight MNE-CPP installer with the Qt Installer Framework.
#
# The generated installer no longer bundles MNE-CPP binaries directly.
# Instead it embeds the selected GitHub release metadata and downloads the
# chosen platform archive during installation.
#
# Usage:
#   ./scripts/build_installer.sh
#   ./scripts/build_installer.sh --tag v2.0.0
#   ./scripts/build_installer.sh --tag latest --asset-prefix mne-cpp-latest

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_DIR="${REPO_ROOT}/build_installer"
STAGING_DIR="${OUTPUT_DIR}/staging"

GH_TAG="dev_build"
ASSET_PREFIX=""

derive_asset_prefix()
{
    case "$1" in
        dev_build)
            printf 'mne-cpp-dev\n'
            ;;
        latest)
            printf 'mne-cpp-latest\n'
            ;;
        *)
            printf 'mne-cpp-%s\n' "$1"
            ;;
    esac
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --tag)
            GH_TAG="$2"
            shift 2
            ;;
        --asset-prefix)
            ASSET_PREFIX="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [[ -z "${ASSET_PREFIX}" ]]; then
    ASSET_PREFIX="$(derive_asset_prefix "${GH_TAG}")"
fi

VERSION_MAJOR="$(sed -n 's/^set(MNE_CPP_VERSION_MAJOR \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
VERSION_MINOR="$(sed -n 's/^set(MNE_CPP_VERSION_MINOR \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
VERSION_PATCH="$(sed -n 's/^set(MNE_CPP_VERSION_PATCH \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
MNE_CPP_VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

if [[ "$OSTYPE" == "darwin"* ]]; then
    INSTALLER_SUFFIX="Darwin"
    PLATFORM="macos"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    INSTALLER_SUFFIX="win64.exe"
    PLATFORM="windows"
else
    INSTALLER_SUFFIX="Linux.run"
    PLATFORM="linux"
fi

echo "-----------------------------------------------------------------"
echo "   MNE-CPP Bootstrap Installer Generation                         "
echo "-----------------------------------------------------------------"
echo ""
echo "  Platform      : ${PLATFORM}"
echo "  Release tag   : ${GH_TAG}"
echo "  Asset prefix  : ${ASSET_PREFIX}"
echo "  Version       : ${MNE_CPP_VERSION}"
echo "  Output dir    : ${OUTPUT_DIR}"
echo ""
echo "  Components:"
echo "    [required] MNE-CPP Core Binaries (dynamic/static selected during install)"
echo "    [opt-in]  MNE Sample Dataset"
echo "    [opt-in]  MNE Python"
echo "    [opt-in]  PATH Configuration"
echo ""

if ! command -v binarycreator >/dev/null 2>&1; then
    echo "WARNING: 'binarycreator' not found in PATH."
    echo "Please ensure Qt Installer Framework is installed and added to PATH."
    echo "Or set 'QtInstallerFramework_DIR'."

    for candidate in \
        "$HOME/Qt/Tools/QtInstallerFramework/4.7/bin" \
        "/opt/Qt/Tools/QtInstallerFramework/4.7/bin" \
        "${QtInstallerFramework_DIR:-}/bin"; do
        if [[ -x "${candidate}/binarycreator" ]]; then
            export PATH="${PATH}:${candidate}"
            echo "Found binarycreator at ${candidate}. Added to PATH."
            break
        fi
    done

    if ! command -v binarycreator >/dev/null 2>&1; then
        echo "ERROR: binarycreator still not found. Aborting."
        exit 1
    fi
fi

echo "1. Preparing bootstrap installer payload..."
"${SCRIPT_DIR}/prepare_bootstrap_installer.sh" \
    --release-tag "${GH_TAG}" \
    --asset-prefix "${ASSET_PREFIX}" \
    --output-dir "${STAGING_DIR}"
echo ""

echo "2. Building installer with binarycreator..."
mkdir -p "${OUTPUT_DIR}"
INSTALLER_FILE="${OUTPUT_DIR}/MNE-CPP-${MNE_CPP_VERSION}-${INSTALLER_SUFFIX}"

binarycreator \
    --offline-only \
    -c "${STAGING_DIR}/config/config.xml" \
    -p "${STAGING_DIR}/packages" \
    "${INSTALLER_FILE}"

echo ""
echo "-----------------------------------------------------------------"
echo "  Success! Installer generated:"
echo "    ${INSTALLER_FILE}"
echo ""
echo "  The installer is a lightweight bootstrapper and downloads the"
echo "  selected MNE-CPP release archive during installation."
echo "-----------------------------------------------------------------"
