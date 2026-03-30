#!/usr/bin/env bash

set -euo pipefail

show_help() {
    cat <<'EOF'
Usage: ./init.sh [options] [-- <extra cmake args>]

Bootstraps a developer build by downloading the MNE-CPP-maintained Qt and Eigen
artifacts into src/external and configuring a build directory from the repo root.

Options:
  --qt-version <version>        Qt version to use (default: 6.11.0)
  --eigen-version <version>     Eigen version to use (default: 5.0.1)
  --linkage <dynamic|static>    Qt linkage / MNE-CPP linkage (default: dynamic)
  --build-type <type>           CMake build type (default: Release)
  --build-dir <path>            Build directory (default: build/developer-<linkage>)
  --qt-dir <path>               Use this existing Qt prefix after validating compatibility
  --qt-mode <auto|system|artifact>
                                `auto`: prefer compatible local Qt, otherwise download artifact
                                `system`: require compatible local Qt
                                `artifact`: always download/use the MNE-CPP Qt artifact
  --eigen-dir <path>            Use an existing Eigen package instead of downloading one
  --with-opengl                 Configure with QOpenGLWidget support instead of the default QRhi-only path
  --repository <owner/repo>     Repository hosting the prerelease assets (default: mne-tools/mne-cpp)
  --qt-release-tag <tag>        Override the Qt prerelease tag
  --eigen-release-tag <tag>     Override the Eigen prerelease tag
  --deps-only                   Only prepare src/external, do not run CMake configure
  --force                       Re-download dependency bundles even if present
  --help                        Show this help text
EOF
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

QT_VERSION="6.11.0"
EIGEN_VERSION="5.0.1"
LINKAGE="dynamic"
BUILD_TYPE="Release"
BUILD_DIR=""
QT_DIR=""
QT_MODE="auto"
EIGEN_DIR=""
REPOSITORY="mne-tools/mne-cpp"
QT_RELEASE_TAG=""
EIGEN_RELEASE_TAG=""
DEPS_ONLY=0
FORCE=0
QT_DIR_EXPLICIT=0
NO_OPENGL_VALUE="ON"
EXTRA_CMAKE_ARGS=()

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
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --qt-dir)
            QT_DIR="$2"
            QT_DIR_EXPLICIT=1
            shift 2
            ;;
        --qt-mode)
            QT_MODE="$2"
            shift 2
            ;;
        --eigen-dir)
            EIGEN_DIR="$2"
            shift 2
            ;;
        --with-opengl)
            NO_OPENGL_VALUE="OFF"
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
        --deps-only)
            DEPS_ONLY=1
            shift
            ;;
        --force)
            FORCE=1
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        --)
            shift
            EXTRA_CMAKE_ARGS=("$@")
            break
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

if [[ "${QT_MODE}" != "auto" && "${QT_MODE}" != "system" && "${QT_MODE}" != "artifact" ]]; then
    echo "--qt-mode must be one of 'auto', 'system', or 'artifact'" >&2
    exit 1
fi

if [[ -z "${EIGEN_DIR}" ]]; then
    EIGEN_DIR="${REPO_ROOT}/src/external/eigen"
fi

if [[ -z "${BUILD_DIR}" ]]; then
    BUILD_DIR="${REPO_ROOT}/build/developer-${LINKAGE}"
fi

for arg in "${EXTRA_CMAKE_ARGS[@]}"; do
    case "${arg}" in
        -DNO_OPENGL=ON|-DNO_OPENGL:BOOL=ON)
            NO_OPENGL_VALUE="ON"
            ;;
        -DNO_OPENGL=OFF|-DNO_OPENGL:BOOL=OFF)
            NO_OPENGL_VALUE="OFF"
            ;;
    esac
done

declare -a QT_CANDIDATES=()
PROBED_QT_ROOT_DIR=""
QT_SOURCE=""
QT_ARTIFACT_DIR="${REPO_ROOT}/src/external/qt/${LINKAGE}"

EIGEN_CONFIG_PATH="${EIGEN_DIR}/share/eigen3/cmake/Eigen3Config.cmake"

add_qt_candidate() {
    local candidate="${1:-}"
    local existing=""

    if [[ -z "${candidate}" ]]; then
        return
    fi

    if [[ -f "${candidate}" ]]; then
        candidate="$(dirname "${candidate}")"
    fi

    if [[ ! -e "${candidate}" ]]; then
        return
    fi

    candidate="$(cd "${candidate}" && pwd)"

    for existing in "${QT_CANDIDATES[@]}"; do
        if [[ "${existing}" == "${candidate}" ]]; then
            return
        fi
    done

    QT_CANDIDATES+=("${candidate}")
}

add_qt_candidates_from_env_path() {
    local raw_list="${1:-}"
    local entry=""

    if [[ -z "${raw_list}" ]]; then
        return
    fi

    while IFS= read -r entry; do
        add_qt_candidate "${entry}"
    done < <(printf '%s' "${raw_list}" | tr ';:' '\n\n')
}

add_qt_candidate_from_query_tool() {
    local tool_name="$1"
    local tool_path=""
    local prefix=""

    if ! tool_path="$(command -v "${tool_name}" 2>/dev/null)"; then
        return
    fi

    if prefix="$("${tool_path}" -query QT_INSTALL_PREFIX 2>/dev/null)"; then
        add_qt_candidate "${prefix}"
        return
    fi

    add_qt_candidate "$(cd "$(dirname "${tool_path}")/.." && pwd)"
}

probe_qt_candidate() {
    local candidate="$1"
    local probe_dir=""
    local result_file=""
    local log_file=""
    local key=""
    local value=""

    probe_dir="$(mktemp -d)"
    result_file="${probe_dir}/result.txt"
    log_file="${probe_dir}/probe.log"

    if cmake \
        -S "${REPO_ROOT}/cmake/qt_probe" \
        -B "${probe_dir}/build" \
        -DMNE_QT_PROBE_CANDIDATE:PATH="${candidate}" \
        -DMNE_QT_PROBE_VERSION:STRING="${QT_VERSION}" \
        -DMNE_QT_PROBE_LINKAGE:STRING="${LINKAGE}" \
        -DMNE_QT_PROBE_NO_OPENGL:BOOL="${NO_OPENGL_VALUE}" \
        -DMNE_QT_PROBE_RESULT_FILE:FILEPATH="${result_file}" \
        >"${log_file}" 2>&1; then
        PROBED_QT_ROOT_DIR=""
        while IFS='=' read -r key value; do
            case "${key}" in
                QT_ROOT_DIR)
                    PROBED_QT_ROOT_DIR="${value}"
                    ;;
            esac
        done < "${result_file}"
        rm -rf "${probe_dir}"
        [[ -n "${PROBED_QT_ROOT_DIR}" ]]
        return
    fi

    echo "Qt probe failed for ${candidate}" >&2
    sed -n '1,120p' "${log_file}" >&2
    rm -rf "${probe_dir}"
    return 1
}

collect_qt_candidates() {
    local home_qt="${HOME}/Qt/${QT_VERSION}"

    add_qt_candidate "${QT_ROOT_DIR:-}"
    add_qt_candidate "${QTDIR:-}"
    add_qt_candidate "${Qt6_DIR:-}"
    add_qt_candidates_from_env_path "${CMAKE_PREFIX_PATH:-}"

    add_qt_candidate_from_query_tool qmake6
    add_qt_candidate_from_query_tool qmake
    add_qt_candidate_from_query_tool qtpaths6
    add_qt_candidate_from_query_tool qtpaths
    add_qt_candidate_from_query_tool qt-cmake

    case "$(uname -s)" in
        Darwin)
            add_qt_candidate "${home_qt}/macos"
            add_qt_candidate "${home_qt}/clang_64"
            add_qt_candidate "/opt/Qt/${QT_VERSION}/macos"
            add_qt_candidate "/opt/Qt/${QT_VERSION}/clang_64"
            add_qt_candidate "/opt/homebrew/opt/qt"
            add_qt_candidate "/usr/local/opt/qt"
            ;;
        Linux)
            add_qt_candidate "${home_qt}/gcc_64"
            add_qt_candidate "/opt/Qt/${QT_VERSION}/gcc_64"
            add_qt_candidate "/usr/lib/qt6"
            add_qt_candidate "/usr/local/qt6"
            add_qt_candidate "/opt/qt6"
            add_qt_candidate "/usr/lib/x86_64-linux-gnu/cmake/Qt6"
            add_qt_candidate "/usr/lib64/cmake/Qt6"
            ;;
    esac
}

resolve_qt_dir() {
    local candidate=""

    if [[ ${QT_DIR_EXPLICIT} -eq 1 ]]; then
        if probe_qt_candidate "${QT_DIR}"; then
            QT_DIR="${PROBED_QT_ROOT_DIR}"
            QT_SOURCE="local"
            return 0
        fi

        echo "The Qt prefix provided via --qt-dir is not compatible with MNE-CPP." >&2
        echo "Expected Qt ${QT_VERSION} (${LINKAGE}) with NO_OPENGL=${NO_OPENGL_VALUE}." >&2
        return 1
    fi

    if [[ "${QT_MODE}" != "artifact" ]]; then
        collect_qt_candidates
        for candidate in "${QT_CANDIDATES[@]}"; do
            if probe_qt_candidate "${candidate}" >/dev/null 2>&1; then
                QT_DIR="${PROBED_QT_ROOT_DIR}"
                QT_SOURCE="local"
                return 0
            fi
        done

        if [[ "${QT_MODE}" == "system" ]]; then
            echo "Could not find a compatible local Qt ${QT_VERSION} for a ${LINKAGE} build." >&2
            return 1
        fi
    fi

    QT_DIR="${QT_ARTIFACT_DIR}"
    QT_SOURCE="artifact"
    return 0
}

if ! resolve_qt_dir; then
    exit 1
fi

external_args=(
    --qt-version "${QT_VERSION}"
    --eigen-version "${EIGEN_VERSION}"
    --linkage "${LINKAGE}"
    --eigen-dir "${EIGEN_DIR}"
    --repository "${REPOSITORY}"
)

if [[ "${QT_SOURCE}" == "artifact" ]]; then
    external_args+=(--qt-dir "${QT_DIR}")
else
    external_args+=(--qt-dir "${QT_DIR}" --skip-qt)
fi

if [[ -n "${QT_RELEASE_TAG}" ]]; then
    external_args+=(--qt-release-tag "${QT_RELEASE_TAG}")
fi

if [[ -n "${EIGEN_RELEASE_TAG}" ]]; then
    external_args+=(--eigen-release-tag "${EIGEN_RELEASE_TAG}")
fi

if [[ ${FORCE} -eq 1 ]]; then
    external_args+=(--force)
fi

"${REPO_ROOT}/src/external/init.sh" "${external_args[@]}"

if [[ ${DEPS_ONLY} -eq 1 ]]; then
    echo ""
    echo "Dependencies are ready in src/external."
    exit 0
fi

cmake_args=(
    -B "${BUILD_DIR}"
    -S "${REPO_ROOT}"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DNO_OPENGL="${NO_OPENGL_VALUE}"
    "-DCMAKE_PREFIX_PATH=${QT_DIR}"
)

if [[ -f "${EIGEN_CONFIG_PATH}" ]]; then
    cmake_args[4]="-DCMAKE_PREFIX_PATH=${QT_DIR};${EIGEN_DIR}"
fi

if [[ "${LINKAGE}" == "static" ]]; then
    cmake_args+=(-DBUILD_SHARED_LIBS=OFF)
fi

if (( ${#EXTRA_CMAKE_ARGS[@]} > 0 )); then
    cmake_args+=("${EXTRA_CMAKE_ARGS[@]}")
fi

cmake "${cmake_args[@]}"

echo ""
echo "Developer configure complete."
echo "  Build directory: ${BUILD_DIR}"
echo "  Qt source: ${QT_SOURCE} (${QT_DIR})"
echo "  NO_OPENGL: ${NO_OPENGL_VALUE}"
echo "  Next step: cmake --build \"${BUILD_DIR}\" --parallel"
