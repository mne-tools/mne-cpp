#!/bin/bash
#
# Prepare a lightweight Qt Installer Framework payload that downloads the
# selected MNE-CPP release archive during installation instead of embedding
# binaries inside the installer itself.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CONFIG_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/config"
CORE_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/bootstrap/org.mnecpp.core"
PACKAGE_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/packages"
PACKAGING_SCRIPTS_DIR="${REPO_ROOT}/scripts/packaging/scripts"

RELEASE_TAG=""
ASSET_PREFIX=""
OUTPUT_DIR=""
RELEASE_DATE="$(date +%F)"

print_help()
{
    cat <<'EOF'
Usage: ./scripts/prepare_bootstrap_installer.sh [options]

Options:
  --release-tag <tag>    GitHub release tag to embed into the installer
  --asset-prefix <name>  Release asset prefix (defaults from the tag)
  --output-dir <dir>     Output staging directory for config/ and packages/
  --release-date <date>  Metadata release date (default: today, YYYY-MM-DD)
  --help                 Show this help
EOF
}

derive_asset_prefix()
{
    local tag="$1"

    case "${tag}" in
        dev_build)
            printf 'mne-cpp-dev\n'
            ;;
        latest)
            printf 'mne-cpp-latest\n'
            ;;
        *)
            printf 'mne-cpp-%s\n' "${tag}"
            ;;
    esac
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --release-tag)
            RELEASE_TAG="$2"
            shift 2
            ;;
        --asset-prefix)
            ASSET_PREFIX="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --release-date)
            RELEASE_DATE="$2"
            shift 2
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            exit 1
            ;;
    esac
done

if [[ -z "${RELEASE_TAG}" ]]; then
    echo "ERROR: --release-tag is required."
    exit 1
fi

if [[ -z "${OUTPUT_DIR}" ]]; then
    echo "ERROR: --output-dir is required."
    exit 1
fi

if [[ -z "${ASSET_PREFIX}" ]]; then
    ASSET_PREFIX="$(derive_asset_prefix "${RELEASE_TAG}")"
fi

VERSION_MAJOR="$(sed -n 's/^set(MNE_CPP_VERSION_MAJOR \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
VERSION_MINOR="$(sed -n 's/^set(MNE_CPP_VERSION_MINOR \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
VERSION_PATCH="$(sed -n 's/^set(MNE_CPP_VERSION_PATCH \([0-9][0-9]*\)).*/\1/p' "${REPO_ROOT}/src/CMakeLists.txt")"
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/config" "${OUTPUT_DIR}/packages"

cp -R "${CONFIG_TEMPLATE_DIR}/." "${OUTPUT_DIR}/config/"
cp -R "${CORE_TEMPLATE_DIR}" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.sampledata" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.mnepython" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.pathconfig" "${OUTPUT_DIR}/packages/"

mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.core/data/scripts"
mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts"
mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts"

cp "${PACKAGING_SCRIPTS_DIR}/install_release_archive.sh" \
   "${OUTPUT_DIR}/packages/org.mnecpp.core/data/scripts/"
cp "${PACKAGING_SCRIPTS_DIR}/install_release_archive.ps1" \
   "${OUTPUT_DIR}/packages/org.mnecpp.core/data/scripts/"
cp "${PACKAGING_SCRIPTS_DIR}/install_mne_python.sh" \
   "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts/"
cp "${PACKAGING_SCRIPTS_DIR}/install_mne_python.bat" \
   "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts/"
cp "${PACKAGING_SCRIPTS_DIR}/configure_environment.sh" \
   "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts/"
cp "${PACKAGING_SCRIPTS_DIR}/configure_environment.bat" \
   "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts/"

find "${OUTPUT_DIR}" -name '.DS_Store' -delete

render_file()
{
    local file="$1"
    local tmp_file="${file}.tmp"

    sed \
        -e "s|@MNE_CPP_VERSION@|${VERSION}|g" \
        -e "s|@MNE_CPP_RELEASE_DATE@|${RELEASE_DATE}|g" \
        -e "s|@MNE_CPP_RELEASE_TAG@|${RELEASE_TAG}|g" \
        -e "s|@MNE_CPP_ASSET_PREFIX@|${ASSET_PREFIX}|g" \
        "${file}" > "${tmp_file}"

    mv "${tmp_file}" "${file}"
}

while IFS= read -r -d '' file; do
    render_file "${file}"
done < <(find "${OUTPUT_DIR}/config" "${OUTPUT_DIR}/packages" -type f \( -name '*.xml' -o -name '*.qs' \) -print0)

echo "Prepared bootstrap installer staging:"
echo "  Release tag : ${RELEASE_TAG}"
echo "  Asset prefix: ${ASSET_PREFIX}"
echo "  Version     : ${VERSION}"
echo "  Output dir  : ${OUTPUT_DIR}"
