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
  --onnxruntime-version <ver>   ONNX Runtime version to download (default: none; set to enable)
  --onnxruntime-dir <path>      Target directory for the ONNX Runtime package
  --onnxruntime-release-tag <t> Override the ONNX Runtime prerelease tag
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
ONNXRUNTIME_VERSION=""
ONNXRUNTIME_DIR=""
REPOSITORY="mne-tools/mne-cpp"
QT_RELEASE_TAG=""
EIGEN_RELEASE_TAG=""
ONNXRUNTIME_RELEASE_TAG=""
FORCE=0
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
        --onnxruntime-version)
            ONNXRUNTIME_VERSION="$2"
            shift 2
            ;;
        --onnxruntime-dir)
            ONNXRUNTIME_DIR="$2"
            shift 2
            ;;
        --onnxruntime-release-tag)
            ONNXRUNTIME_RELEASE_TAG="$2"
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


# Candidate Qt roots: --qt-dir, env vars, common locations
QT_CANDIDATES=()
if [[ -n "${QT_DIR}" ]]; then
    QT_CANDIDATES+=("${QT_DIR}")
fi
if [[ -n "${QT_ROOT_DIR:-}" ]]; then
    QT_CANDIDATES+=("${QT_ROOT_DIR}")
fi
if [[ -n "${Qt6_DIR:-}" ]]; then
    QT_CANDIDATES+=("${Qt6_DIR}")
fi
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
    IFS=":" read -ra CPATHS <<< "${CMAKE_PREFIX_PATH}"
    for p in "${CPATHS[@]}"; do
        QT_CANDIDATES+=("$p")
    done
fi
# Add common install locations
QT_CANDIDATES+=("/usr/local/opt/qt6" "/usr/local/Qt-6" "/opt/Qt6" "/usr/local/Qt6" "/usr/lib/qt6" "/usr/local/Cellar/qt@6" "/usr/local/qt6")

# Probe for compatible Qt using a minimal CMake project
QT_PROBE_SUCCESS=0
QT_PROBE_DIR="$(mktemp -d)"
trap 'rm -rf "${QT_PROBE_DIR}"' EXIT
cp "${SCRIPT_DIR}/qt_probe.cmake" "${QT_PROBE_DIR}/CMakeLists.txt"

for candidate in "${QT_CANDIDATES[@]}"; do
    if [[ -d "$candidate/lib/cmake/Qt6" ]]; then
        if cmake -S "${QT_PROBE_DIR}" -B "${QT_PROBE_DIR}/_build" \
                 -DCMAKE_PREFIX_PATH="$candidate" \
                 -DQt6_DIR="$candidate/lib/cmake/Qt6" >/dev/null 2>&1; then
            QT_PROBE_SUCCESS=1
            QT_DIR="$candidate"
            break
        fi
        rm -rf "${QT_PROBE_DIR}/_build"
    fi
done
rm -rf "${QT_PROBE_DIR}"

if [[ $QT_PROBE_SUCCESS -eq 0 ]]; then
    QT_DIR="${SCRIPT_DIR}/qt/${LINKAGE}"
fi

if [[ -z "${EIGEN_DIR}" ]]; then
    EIGEN_DIR="${SCRIPT_DIR}/eigen"
fi

if [[ -n "${ONNXRUNTIME_VERSION}" && -z "${ONNXRUNTIME_DIR}" ]]; then
    ONNXRUNTIME_DIR="${SCRIPT_DIR}/onnxruntime"
fi

mkdir -p "${SCRIPT_DIR}/qt"


# Only download if not using a compatible system Qt
download_qt=1
if [[ ${SKIP_QT} -eq 1 ]]; then
    download_qt=0
elif [[ ${FORCE} -eq 0 && -f "${QT_DIR}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    # If we just probed a system Qt, don't download
    if [[ $QT_PROBE_SUCCESS -eq 1 ]]; then
        download_qt=0
    fi
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
elif [[ $QT_PROBE_SUCCESS -eq 1 ]]; then
    echo "Using compatible system Qt at ${QT_DIR} (no artifact download needed)"
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
    "${REPO_ROOT}/scripts/ci/download_toolchain.sh" "${eigen_args[@]}"
else
    echo "Eigen bundle already present at ${EIGEN_DIR}"
fi

if [[ -n "${ONNXRUNTIME_VERSION}" ]]; then
    download_onnx=1
    if [[ ${FORCE} -eq 0 && -f "${ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h" ]]; then
        download_onnx=0
    fi
    if [[ ${download_onnx} -eq 1 ]]; then
        ort_args=(
            --kind onnxruntime
            --version "${ONNXRUNTIME_VERSION}"
            --output-dir "${ONNXRUNTIME_DIR}"
            --repository "${REPOSITORY}"
        )
        if [[ -n "${ONNXRUNTIME_RELEASE_TAG}" ]]; then
            ort_args+=(--release-tag "${ONNXRUNTIME_RELEASE_TAG}")
        fi
        "${REPO_ROOT}/scripts/ci/download_toolchain.sh" "${ort_args[@]}"
    else
        echo "ONNX Runtime already present at ${ONNXRUNTIME_DIR}"
    fi
fi

echo ""
echo "Dependency setup complete."
if [[ ${SKIP_QT} -eq 1 ]]; then
    echo "  Qt:    ${QT_DIR} (caller-provided/local)"
else
    echo "  Qt:    ${QT_DIR}"
fi
echo "  Eigen: ${EIGEN_DIR}"
if [[ -n "${ONNXRUNTIME_VERSION}" ]]; then
    echo "  ONNX Runtime: ${ONNXRUNTIME_DIR}"
fi
echo "  CMake prefix hint: ${QT_DIR};${EIGEN_DIR}"
