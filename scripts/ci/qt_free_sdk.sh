#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Proves that MNE-CPP's libraries build, test, install and are consumed without Qt:
#   1. configures a clean build with Qt hidden from CMake (MNE_BUILD_QT switches itself off),
#   2. builds the Qt-free libraries, examples and tests and runs the tests,
#   3. installs the SDK and checks that neither its CMake package nor its libraries refer to Qt,
#   4. builds and runs scripts/ci/sdk_consumer against the install tree alone.
#
# Usage: scripts/ci/qt_free_sdk.sh [work dir] [extra CMake arguments...]
#   The work dir (default: build/qt-free) receives build/, sdk/ and consumer/.
#   Eigen is looked up in src/3rdparty/eigen, as for every other build.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK_DIR="${1:-$REPO_ROOT/build/qt-free}"
if command -v cygpath >/dev/null; then
    # Git Bash on Windows: hand CMake native paths.
    REPO_ROOT="$(cygpath -m "$REPO_ROOT")"
    WORK_DIR="$(cygpath -m "$WORK_DIR")"
fi
shift || true
BUILD_DIR="$WORK_DIR/build"
SDK_DIR="$WORK_DIR/sdk"
CONSUMER_DIR="$WORK_DIR/consumer"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

rm -rf "$BUILD_DIR" "$SDK_DIR" "$CONSUMER_DIR"
mkdir -p "$WORK_DIR"

# No prefix path from the environment, and find_package(Qt6) disabled even where Qt is installed system-wide.
unset CMAKE_PREFIX_PATH Qt6_DIR QT_DIR
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON \
    -DBINARY_OUTPUT_DIRECTORY="$BUILD_DIR/out" \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON \
    "$@" 2>&1 | tee "$WORK_DIR/configure.log"

grep -q "MNE_BUILD_QT is OFF" "$WORK_DIR/configure.log" ||
    { echo "error: the configure step did not report a Qt-free build" >&2; exit 1; }

cmake --build "$BUILD_DIR" --config Release --parallel "$JOBS"
ctest --test-dir "$BUILD_DIR" -C Release --output-on-failure --no-tests=error --parallel "$JOBS"

for component in runtime sdk; do
    cmake --install "$BUILD_DIR" --config Release --prefix "$SDK_DIR" --component "$component"
done

# The installed package must neither look for Qt nor hand it to its users.
if grep -rIlE "Qt[56]?::|find_dependency\(Qt" "$SDK_DIR/lib/cmake/MNE-CPP"; then
    echo "error: the installed MNE-CPP package refers to Qt (files above)" >&2
    exit 1
fi

# ... and no installed library or program may load a Qt library (the loader names it in every format).
status=0
while IFS= read -r -d '' binary; do
    if LC_ALL=C grep -a -q -E "libQt[56]Core|Qt[56]Core\.dll|QtCore\.framework" "$binary"; then
        echo "error: $binary links Qt" >&2
        status=1
    fi
done < <(find "$SDK_DIR" -type f \( -name "*.so*" -o -name "*.dylib" -o -name "*.dll" -o -name "*.exe" \
                                  -o -path "*/bin/*" \) -print0)
[[ $status -eq 0 ]] || exit 1

cmake -S "$REPO_ROOT/scripts/ci/sdk_consumer" -B "$CONSUMER_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SDK_DIR" \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
cmake --build "$CONSUMER_DIR" --config Release
ctest --test-dir "$CONSUMER_DIR" -C Release --output-on-failure --no-tests=error

echo "Qt-free SDK: built, tested, installed to $SDK_DIR and consumed."
