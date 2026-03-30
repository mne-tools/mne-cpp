#!/usr/bin/env bash

set -euo pipefail

show_help() {
    cat <<'EOF'
Usage: ./src/external/init.sh [options]

Downloads the MNE-CPP-maintained Qt and Eigen dependency bundles into src/external.

Options:
  --qt-version <version>        Qt version to download (default: 6.11.0)
  --eigen-version <version>     Eigen version to download (default: 5.0.1)
  --linkage <dynamic|static>    Qt linkage to prepare (default: dynamic)
  --qt-dir <path>               Target directory for the Qt bundle
  --eigen-dir <path>            Target directory for the Eigen bundle
  --skip-qt                     Skip Qt download and keep using the provided/local Qt prefix
  --repository <owner/repo>     Repository hosting the prerelease assets (default: mne-tools/mne-cpp)
  --qt-release-tag <tag>        Override the Qt prerelease tag
  --eigen-release-tag <tag>     Override the Eigen prerelease tag
  --force                       Re-download even if the expected package already exists
  --help                        Show this help text
EOF
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

QT_VERSION="6.11.0"
EIGEN_VERSION="5.0.1"
LINKAGE="dynamic"
QT_DIR=""
EIGEN_DIR=""
REPOSITORY="mne-tools/mne-cpp"
QT_RELEASE_TAG=""
EIGEN_RELEASE_TAG=""
FORCE=0
BUNDLED_EIGEN_DIR=""
SKIP_QT=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --qt-version)
            QT_VERSION="$2"
            shift 2
            ;;
        --eigen-version)
            EIGEN_VERSION="$2"
            shift 2
            ;;
        --linkage)
            LINKAGE="$2"
            shift 2
            ;;
        --qt-dir)
            QT_DIR="$2"
            shift 2
            ;;
        --eigen-dir)
            EIGEN_DIR="$2"
            shift 2
            ;;
        --skip-qt)
            SKIP_QT=1
            shift
            ;;
        --repository)
            REPOSITORY="$2"
            shift 2
            ;;
        --qt-release-tag)
            QT_RELEASE_TAG="$2"
            shift 2
            ;;
        --eigen-release-tag)
            EIGEN_RELEASE_TAG="$2"
            shift 2
            ;;
        --force)
            FORCE=1
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            show_help
            exit 1
            ;;
    esac
done

if [[ "${LINKAGE}" != "dynamic" && "${LINKAGE}" != "static" ]]; then
    echo "--linkage must be either 'dynamic' or 'static'" >&2
    exit 1
fi

if [[ -z "${QT_DIR}" ]]; then
    QT_DIR="${SCRIPT_DIR}/qt/${LINKAGE}"
fi

if [[ -z "${EIGEN_DIR}" ]]; then
    EIGEN_DIR="${SCRIPT_DIR}/eigen"
fi

BUNDLED_EIGEN_DIR="${SCRIPT_DIR}/eigen-${EIGEN_VERSION}"

mkdir -p "${SCRIPT_DIR}/qt"

download_qt=1
if [[ ${SKIP_QT} -eq 1 ]]; then
    download_qt=0
elif [[ ${FORCE} -eq 0 && -f "${QT_DIR}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    download_qt=0
fi

download_eigen=1
if [[ ${FORCE} -eq 0 && -f "${EIGEN_DIR}/share/eigen3/cmake/Eigen3Config.cmake" ]]; then
    download_eigen=0
fi

if [[ ${download_qt} -eq 1 ]]; then
    qt_args=(
        --kind qt
        --version "${QT_VERSION}"
        --linkage "${LINKAGE}"
        --output-dir "${QT_DIR}"
        --repository "${REPOSITORY}"
    )
    if [[ -n "${QT_RELEASE_TAG}" ]]; then
        qt_args+=(--release-tag "${QT_RELEASE_TAG}")
    fi
    "${REPO_ROOT}/scripts/ci/download_toolchain.sh" "${qt_args[@]}"
elif [[ ${SKIP_QT} -eq 1 ]]; then
    if [[ -n "${QT_DIR}" ]]; then
        echo "Skipping Qt artifact download. Using caller-provided/local Qt at ${QT_DIR}"
    else
        echo "Skipping Qt artifact download. Expecting Qt to be supplied by the caller."
    fi
else
    echo "Qt bundle already present at ${QT_DIR}"
fi

if [[ ${download_eigen} -eq 1 ]]; then
    eigen_args=(
        --kind eigen
        --version "${EIGEN_VERSION}"
        --output-dir "${EIGEN_DIR}"
        --repository "${REPOSITORY}"
    )
    if [[ -n "${EIGEN_RELEASE_TAG}" ]]; then
        eigen_args+=(--release-tag "${EIGEN_RELEASE_TAG}")
    fi
    if "${REPO_ROOT}/scripts/ci/download_toolchain.sh" "${eigen_args[@]}"; then
        :
    elif [[ -f "${BUNDLED_EIGEN_DIR}/CMakeLists.txt" ]]; then
        echo "Eigen artifact is not available in the public prerelease yet. Continuing with bundled fallback at ${BUNDLED_EIGEN_DIR}."
    else
        echo "Failed to prepare Eigen and no bundled fallback was found at ${BUNDLED_EIGEN_DIR}." >&2
        exit 1
    fi
else
    echo "Eigen bundle already present at ${EIGEN_DIR}"
fi

echo ""
echo "Dependency setup complete."
if [[ ${SKIP_QT} -eq 1 ]]; then
    echo "  Qt:    ${QT_DIR} (caller-provided/local)"
else
    echo "  Qt:    ${QT_DIR}"
fi
if [[ -f "${EIGEN_DIR}/share/eigen3/cmake/Eigen3Config.cmake" ]]; then
    echo "  Eigen: ${EIGEN_DIR}"
    echo "  CMake prefix hint: ${QT_DIR};${EIGEN_DIR}"
else
    echo "  Eigen: bundled fallback at ${BUNDLED_EIGEN_DIR}"
    echo "  CMake prefix hint: ${QT_DIR}"
fi
