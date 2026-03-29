#!/bin/bash
#
# install_release_archive.sh
# Download and extract an MNE-CPP release archive into the installer target.

set -euo pipefail

URL=""
TARGET_DIR=""
ASSET_NAME=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --url)
            URL="$2"
            shift 2
            ;;
        --target-dir)
            TARGET_DIR="$2"
            shift 2
            ;;
        --asset-name)
            ASSET_NAME="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "${URL}" || -z "${TARGET_DIR}" || -z "${ASSET_NAME}" ]]; then
    echo "ERROR: Missing required arguments."
    echo "Usage: install_release_archive.sh --url <url> --target-dir <dir> --asset-name <file>"
    exit 1
fi

download_file()
{
    local url="$1"
    local archive="$2"

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail --progress-bar -o "$archive" "$url"
        return
    fi

    if command -v wget >/dev/null 2>&1; then
        wget --show-progress -O "$archive" "$url"
        return
    fi

    echo "ERROR: Neither curl nor wget is available."
    exit 1
}

find_payload_root()
{
    local root="$1"

    if [[ -d "${root}/bin" || -d "${root}/apps" ]]; then
        printf '%s\n' "$root"
        return
    fi

    local candidate
    candidate=$(find "$root" -mindepth 1 -maxdepth 4 -type d \( -name bin -o -name apps \) | head -n 1 || true)
    if [[ -n "${candidate}" ]]; then
        dirname "$candidate"
        return
    fi

    printf '%s\n' "$root"
}

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/mne-cpp-installer.XXXXXX")
ARCHIVE_PATH="${TMP_DIR}/${ASSET_NAME}"
EXTRACT_DIR="${TMP_DIR}/extract"
mkdir -p "${EXTRACT_DIR}"

cleanup()
{
    rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

echo "================================================================"
echo "  MNE-CPP: Downloading selected release archive"
echo "================================================================"
echo "URL       : ${URL}"
echo "Asset     : ${ASSET_NAME}"
echo "Target    : ${TARGET_DIR}"
echo ""

download_file "${URL}" "${ARCHIVE_PATH}"

echo ""
echo "Extracting release archive..."

case "${ASSET_NAME}" in
    *.tar.gz|*.tgz)
        tar -xzf "${ARCHIVE_PATH}" -C "${EXTRACT_DIR}"
        ;;
    *.zip)
        if command -v unzip >/dev/null 2>&1; then
            unzip -q "${ARCHIVE_PATH}" -d "${EXTRACT_DIR}"
        else
            echo "ERROR: unzip is required for ${ASSET_NAME}."
            exit 1
        fi
        ;;
    *)
        echo "ERROR: Unsupported archive type: ${ASSET_NAME}"
        exit 1
        ;;
esac

PAYLOAD_ROOT=$(find_payload_root "${EXTRACT_DIR}")

echo "Payload root: ${PAYLOAD_ROOT}"
mkdir -p "${TARGET_DIR}"

# Merge the extracted payload into the chosen target while preserving
# executable bits, symlinks, and macOS bundle structure.
(cd "${PAYLOAD_ROOT}" && tar -cf - .) | (cd "${TARGET_DIR}" && tar -xpf -)

echo ""
echo "MNE-CPP release archive installed successfully."
