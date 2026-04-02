#!/bin/bash
#
# Prepare a Qt Installer Framework offline-installer payload by hydrating the
# platform-specific dynamic and static MNE-CPP release archives into IFW
# packages. The installer binary is self-contained and includes all components.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CONFIG_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/config"
CORE_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/bootstrap/org.mnecpp.core"
PACKAGE_TEMPLATE_DIR="${REPO_ROOT}/scripts/packaging/installer/packages"

RELEASE_TAG=""
ASSET_PREFIX=""
OUTPUT_DIR=""
RELEASE_DATE="$(date +%F)"
PLATFORM=""
DYNAMIC_ARCHIVE=""
STATIC_ARCHIVE=""
GITHUB_REPOSITORY_NAME="${GITHUB_REPOSITORY:-mne-tools/mne-cpp}"

print_help()
{
    cat <<'EOF'
Usage: ./scripts/prepare_bootstrap_installer.sh [options]

Options:
  --release-tag <tag>       GitHub release tag to embed into the installer
  --asset-prefix <name>     Release asset prefix (defaults from the tag)
  --platform <name>         linux | macos | windows
  --output-dir <dir>        Output staging directory for config/ and packages/
  --release-date <date>     Metadata release date (default: today, YYYY-MM-DD)
  --dynamic-archive <path>  Optional pre-downloaded dynamic release archive
  --static-archive <path>   Optional pre-downloaded static release archive
  --help                    Show this help
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

release_asset_name()
{
    local variant="$1"

    case "${PLATFORM}" in
        linux)
            printf '%s-linux-%s-x86_64.tar.gz\n' "${ASSET_PREFIX}" "${variant}"
            ;;
        macos)
            printf '%s-macos-%s-arm64.tar.gz\n' "${ASSET_PREFIX}" "${variant}"
            ;;
        windows)
            printf '%s-windows-%s-x86_64.zip\n' "${ASSET_PREFIX}" "${variant}"
            ;;
        *)
            echo "ERROR: Unsupported platform '${PLATFORM}'." >&2
            exit 1
            ;;
    esac
}

find_payload_root()
{
    local root="$1"

    if [[ -d "${root}/bin" || -d "${root}/apps" ]]; then
        printf '%s\n' "${root}"
        return
    fi

    local candidate
    candidate=$(find "${root}" -mindepth 1 -maxdepth 4 -type d \( -name bin -o -name apps \) | head -n 1 || true)
    if [[ -n "${candidate}" ]]; then
        dirname "${candidate}"
        return
    fi

    printf '%s\n' "${root}"
}

extract_payload_archive()
{
    local archive_path="$1"
    local destination_dir="$2"
    local extract_dir="$3"

    rm -rf "${extract_dir}"
    mkdir -p "${extract_dir}" "${destination_dir}"

    case "${archive_path}" in
        *.tar.gz|*.tgz)
            tar -xzf "${archive_path}" -C "${extract_dir}"
            ;;
        *.zip)
            unzip -q "${archive_path}" -d "${extract_dir}"
            ;;
        *)
            echo "ERROR: Unsupported archive type '${archive_path}'." >&2
            exit 1
            ;;
    esac

    local payload_root
    payload_root="$(find_payload_root "${extract_dir}")"
    (cd "${payload_root}" && tar -cf - .) | (cd "${destination_dir}" && tar -xf -)
}

download_release_asset()
{
    local asset_name="$1"
    local destination_dir="$2"

    if ! command -v gh >/dev/null 2>&1; then
        echo "ERROR: gh CLI is required to download ${asset_name}." >&2
        exit 1
    fi

    gh release download "${RELEASE_TAG}" \
        -R "${GITHUB_REPOSITORY_NAME}" \
        -p "${asset_name}" \
        -D "${destination_dir}"
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
        --platform)
            PLATFORM="$2"
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
        --dynamic-archive)
            DYNAMIC_ARCHIVE="$2"
            shift 2
            ;;
        --static-archive)
            STATIC_ARCHIVE="$2"
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

if [[ -z "${PLATFORM}" ]]; then
    echo "ERROR: --platform is required."
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

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/mne-cpp-installer.XXXXXX")"
trap 'rm -rf "${TMP_DIR}"' EXIT

DYNAMIC_ASSET_NAME="$(release_asset_name dynamic)"
STATIC_ASSET_NAME="$(release_asset_name static)"

if [[ -z "${DYNAMIC_ARCHIVE}" ]]; then
    download_release_asset "${DYNAMIC_ASSET_NAME}" "${TMP_DIR}"
    DYNAMIC_ARCHIVE="${TMP_DIR}/${DYNAMIC_ASSET_NAME}"
fi

if [[ -z "${STATIC_ARCHIVE}" ]]; then
    download_release_asset "${STATIC_ASSET_NAME}" "${TMP_DIR}"
    STATIC_ARCHIVE="${TMP_DIR}/${STATIC_ASSET_NAME}"
fi

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/config" "${OUTPUT_DIR}/packages"

cp -R "${CONFIG_TEMPLATE_DIR}/." "${OUTPUT_DIR}/config/"
cp -R "${CORE_TEMPLATE_DIR}" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.sampledata" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.mnepython" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.pathconfig" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.payload.dynamic" "${OUTPUT_DIR}/packages/"
cp -R "${PACKAGE_TEMPLATE_DIR}/org.mnecpp.payload.static" "${OUTPUT_DIR}/packages/"

mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.payload.dynamic/data"
mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.payload.static/data"
mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts"
mkdir -p "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts"

cp "${REPO_ROOT}/scripts/packaging/scripts/install_mne_python.sh" \
   "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts/"
cp "${REPO_ROOT}/scripts/packaging/scripts/install_mne_python.bat" \
   "${OUTPUT_DIR}/packages/org.mnecpp.mnepython/data/scripts/"
cp "${REPO_ROOT}/scripts/packaging/scripts/configure_environment.sh" \
   "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts/"
cp "${REPO_ROOT}/scripts/packaging/scripts/configure_environment.bat" \
   "${OUTPUT_DIR}/packages/org.mnecpp.pathconfig/data/scripts/"

extract_payload_archive \
    "${DYNAMIC_ARCHIVE}" \
    "${OUTPUT_DIR}/packages/org.mnecpp.payload.dynamic/data" \
    "${TMP_DIR}/extract-dynamic"

extract_payload_archive \
    "${STATIC_ARCHIVE}" \
    "${OUTPUT_DIR}/packages/org.mnecpp.payload.static/data" \
    "${TMP_DIR}/extract-static"

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

echo "Prepared installer staging:"
echo "  Release tag    : ${RELEASE_TAG}"
echo "  Asset prefix   : ${ASSET_PREFIX}"
echo "  Platform       : ${PLATFORM}"
echo "  Version        : ${VERSION}"
echo "  Output dir     : ${OUTPUT_DIR}"
