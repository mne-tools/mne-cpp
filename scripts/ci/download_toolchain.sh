#!/usr/bin/env bash

set -euo pipefail

usage()
{
    cat <<'EOF'
Usage: ./scripts/ci/download_toolchain.sh --kind <qt|ifw|eigen|onnxruntime> --version <version> --output-dir <dir> [--linkage <dynamic|static>] [--platform <platform>] [--release-tag <tag>] [--repository <owner/repo>]
EOF
}

append_path()
{
    local path_entry="$1"
    if [[ -n "${GITHUB_PATH:-}" ]]; then
        printf '%s\n' "${path_entry}" >> "${GITHUB_PATH}"
    fi
    export PATH="${path_entry}:${PATH}"
}

persist_env()
{
    local env_name="$1"
    local env_value="$2"
    if [[ -n "${GITHUB_ENV:-}" ]]; then
        printf '%s=%s\n' "${env_name}" "${env_value}" >> "${GITHUB_ENV}"
    fi
    export "${env_name}=${env_value}"
}

download_release_asset()
{
    local asset_name="$1"
    local destination_dir="$2"
    local max_retries=4
    local delay=10

    for attempt in $(seq 1 "${max_retries}"); do
        if command -v gh >/dev/null 2>&1; then
            if [[ -n "${GH_TOKEN:-}" || -n "${GITHUB_TOKEN:-}" ]]; then
                if gh release download "${RELEASE_TAG}" -R "${REPOSITORY}" -p "${asset_name}" -D "${destination_dir}" >/dev/null 2>&1; then
                    return 0
                fi
            elif gh auth status >/dev/null 2>&1; then
                if gh release download "${RELEASE_TAG}" -R "${REPOSITORY}" -p "${asset_name}" -D "${destination_dir}" >/dev/null 2>&1; then
                    return 0
                fi
            fi
        fi

        local asset_url="https://github.com/${REPOSITORY}/releases/download/${RELEASE_TAG}/${asset_name}"
        if curl --fail --location --silent --show-error \
            --output "${destination_dir}/${asset_name}" \
            "${asset_url}"; then
            return 0
        fi

        if [[ "${attempt}" -lt "${max_retries}" ]]; then
            echo "Download attempt ${attempt}/${max_retries} failed, retrying in ${delay}s..."
            sleep "${delay}"
            delay=$((delay * 2))
        fi
    done

    echo "ERROR: Failed to download ${asset_name} after ${max_retries} attempts."
    return 1
}

KIND=""
VERSION=""
LINKAGE=""
OUTPUT_DIR=""
RELEASE_TAG=""
PLATFORM_OVERRIDE=""
REPOSITORY="${GITHUB_REPOSITORY:-mne-tools/mne-cpp}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --kind)
            KIND="$2"
            shift 2
            ;;
        --version)
            VERSION="$2"
            shift 2
            ;;
        --linkage)
            LINKAGE="$2"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --release-tag)
            RELEASE_TAG="$2"
            shift 2
            ;;
        --platform)
            PLATFORM_OVERRIDE="$2"
            shift 2
            ;;
        --repository)
            REPOSITORY="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ -z "${KIND}" || -z "${VERSION}" || -z "${OUTPUT_DIR}" ]]; then
    usage
    exit 1
fi

case "$(uname -s)" in
    Linux)
        PLATFORM="linux"
        ARCHIVE_EXTENSION="tar.gz"
        PATH_SEPARATOR=":"
        ;;
    Darwin)
        PLATFORM="macos"
        ARCHIVE_EXTENSION="tar.gz"
        PATH_SEPARATOR=":"
        ;;
    *)
        echo "Unsupported platform for download_toolchain.sh" >&2
        exit 1
        ;;
esac

if [[ -n "${PLATFORM_OVERRIDE}" ]]; then
    PLATFORM="${PLATFORM_OVERRIDE}"
fi

VERSION_TOKEN="${VERSION//./}"

case "${KIND}" in
    qt)
        if [[ -z "${RELEASE_TAG}" ]]; then
            RELEASE_TAG="${QT_TOOLCHAIN_RELEASE_TAG:-qt_binaries}"
        fi
        if [[ -z "${LINKAGE}" ]]; then
            echo "--linkage is required when --kind=qt" >&2
            exit 1
        fi
        ASSET_NAME="qt6_${VERSION_TOKEN}_${LINKAGE}_binaries_${PLATFORM}.${ARCHIVE_EXTENSION}"
        ;;
    ifw)
        if [[ -z "${RELEASE_TAG}" ]]; then
            RELEASE_TAG="${QT_TOOLCHAIN_RELEASE_TAG:-qt_binaries}"
        fi
        ASSET_NAME="qt_ifw_${VERSION_TOKEN}_${PLATFORM}.${ARCHIVE_EXTENSION}"
        ;;
    eigen)
        if [[ -z "${RELEASE_TAG}" ]]; then
            RELEASE_TAG="${EIGEN_TOOLCHAIN_RELEASE_TAG:-eigen_artifacts}"
        fi
        ASSET_NAME="eigen_${VERSION_TOKEN}_any.${ARCHIVE_EXTENSION}"
        ;;
    onnxruntime)
        # Re-hosted on mne-tools/mne-cpp releases (same as Qt/Eigen).
        # The buildonnxruntimeartifacts.yml workflow fetches the official
        # Microsoft binaries, verifies them and re-packages under our
        # naming convention: onnxruntime_<ver>_<platform>[_<arch>].<ext>
        if [[ -z "${RELEASE_TAG}" ]]; then
            RELEASE_TAG="${ONNXRUNTIME_TOOLCHAIN_RELEASE_TAG:-onnxruntime_artifacts}"
        fi
        case "${PLATFORM}" in
            linux)
                ASSET_NAME="onnxruntime_${VERSION_TOKEN}_linux.${ARCHIVE_EXTENSION}"
                ;;
            macos)
                if [[ "$(uname -m)" == "arm64" ]]; then
                    ASSET_NAME="onnxruntime_${VERSION_TOKEN}_macos_arm64.${ARCHIVE_EXTENSION}"
                else
                    ASSET_NAME="onnxruntime_${VERSION_TOKEN}_macos_x86_64.${ARCHIVE_EXTENSION}"
                fi
                ;;
            windows)
                ASSET_NAME="onnxruntime_${VERSION_TOKEN}_windows.${ARCHIVE_EXTENSION}"
                ;;
            wasm)
                ASSET_NAME="onnxruntime_${VERSION_TOKEN}_wasm.${ARCHIVE_EXTENSION}"
                ;;
        esac
        ;;
    *)
        echo "Unsupported toolchain kind: ${KIND}" >&2
        exit 1
        ;;
esac

DOWNLOAD_DIR="$(mktemp -d)"
trap 'rm -rf "${DOWNLOAD_DIR}"' EXIT

echo "Downloading ${ASSET_NAME} from release ${RELEASE_TAG} (${REPOSITORY})..."
download_release_asset "${ASSET_NAME}" "${DOWNLOAD_DIR}"

rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"
tar xzf "${DOWNLOAD_DIR}/${ASSET_NAME}" -C "${OUTPUT_DIR}"

if [[ "${KIND}" == "qt" ]]; then
    QT_ROOT_DIR="${OUTPUT_DIR}"
    QT6_CONFIG_DIR="${QT_ROOT_DIR}/lib/cmake/Qt6"
    CMAKE_PREFIX_VALUE="${QT_ROOT_DIR}"
    if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
        CMAKE_PREFIX_VALUE="${QT_ROOT_DIR}${PATH_SEPARATOR}${CMAKE_PREFIX_PATH}"
    fi

    persist_env "QT_ROOT_DIR" "${QT_ROOT_DIR}"
    persist_env "CMAKE_PREFIX_PATH" "${CMAKE_PREFIX_VALUE}"
    if [[ -d "${QT6_CONFIG_DIR}" ]]; then
        persist_env "Qt6_DIR" "${QT6_CONFIG_DIR}"
    else
        persist_env "Qt6_DIR" "${QT_ROOT_DIR}"
    fi
    append_path "${QT_ROOT_DIR}/bin"
    echo "Qt toolchain ready at ${QT_ROOT_DIR}"
elif [[ "${KIND}" == "ifw" ]]; then
    persist_env "QtInstallerFramework_DIR" "${OUTPUT_DIR}"
    persist_env "CPACK_IFW_ROOT" "${OUTPUT_DIR}"
    append_path "${OUTPUT_DIR}/bin"
    echo "Qt Installer Framework ready at ${OUTPUT_DIR}"
elif [[ "${KIND}" == "onnxruntime" ]]; then
    persist_env "ONNXRUNTIME_ROOT_DIR" "${OUTPUT_DIR}"
    echo "ONNX Runtime ready at ${OUTPUT_DIR}"
else
    EIGEN_ROOT_DIR="${OUTPUT_DIR}"
    EIGEN3_CONFIG_DIR="${EIGEN_ROOT_DIR}/share/eigen3/cmake"
    CMAKE_PREFIX_VALUE="${EIGEN_ROOT_DIR}"
    if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
        CMAKE_PREFIX_VALUE="${EIGEN_ROOT_DIR}${PATH_SEPARATOR}${CMAKE_PREFIX_PATH}"
    fi

    persist_env "EIGEN3_ROOT_DIR" "${EIGEN_ROOT_DIR}"
    persist_env "CMAKE_PREFIX_PATH" "${CMAKE_PREFIX_VALUE}"
    if [[ -d "${EIGEN3_CONFIG_DIR}" ]]; then
        persist_env "Eigen3_DIR" "${EIGEN3_CONFIG_DIR}"
    else
        persist_env "Eigen3_DIR" "${EIGEN_ROOT_DIR}"
    fi
    echo "Eigen package ready at ${EIGEN_ROOT_DIR}"
fi
