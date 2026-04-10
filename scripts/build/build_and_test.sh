#!/usr/bin/env bash
# ==========================================================================
#  MNE-CPP  —  Configure ▸ Clean Build ▸ Test ▸ (optional) Coverage
#
#  Usage:
#    ./scripts/build/build_and_test.sh                # Release build, run tests
#    ./scripts/build/build_and_test.sh coverage       # Same but with gcov coverage
#    ./scripts/build/build_and_test.sh Debug          # Debug build, run tests
#    ./scripts/build/build_and_test.sh Debug coverage # Debug + coverage
#    ./scripts/build/build_and_test.sh help           # Show help
#
#  The script:
#    1. Ensures mne-cpp-test-data is cloned
#    2. Ensures MNE sample data is available (via mne-python or curl)
#    3. Deletes previous build/out dirs (clean build)
#    4. Configures CMake with BUILD_TESTS=ON (and WITH_CODE_COV if requested)
#    5. Builds with all available cores
#    6. Runs all tests via scripts/test/test_all.bat
#    7. If coverage: generates lcov/fastcov report
# ==========================================================================
set -euo pipefail

# ── Resolve paths ─────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd -P)"

# ── Defaults ──────────────────────────────────────────────────────────────
BUILD_TYPE="Release"
WITH_COVERAGE="false"
PRINT_HELP="false"
EXTRA_CMAKE_FLAGS=""

# ── Parse arguments ───────────────────────────────────────────────────────
for arg in "$@"; do
  case "$arg" in
    help|--help|-h)
      PRINT_HELP="true"
      ;;
    coverage)
      WITH_COVERAGE="true"
      ;;
    Debug*|debug)
      BUILD_TYPE="Debug"
      ;;
    Release*|release)
      BUILD_TYPE="Release"
      ;;
    --)
      shift
      EXTRA_CMAKE_FLAGS="$*"
      break
      ;;
  esac
done

# ── Help ──────────────────────────────────────────────────────────────────
show_help() {
  cat <<'EOF'

  MNE-CPP Build & Test convenience script

  Usage: ./scripts/build/build_and_test.sh [options]

  Options:
    help        Show this help and exit
    coverage    Enable gcov code coverage (slower build & run)
    Debug       Use Debug build type (default: Release)
    Release     Use Release build type (explicit)
    --          Everything after this is forwarded to cmake

  Examples:
    ./scripts/build/build_and_test.sh                  # Quick Release build + tests
    ./scripts/build/build_and_test.sh coverage         # Release + coverage analysis
    ./scripts/build/build_and_test.sh Debug coverage   # Debug + coverage analysis

EOF
}

if [ "$PRINT_HELP" = "true" ]; then
  show_help
  exit 0
fi

# ── Derived paths ─────────────────────────────────────────────────────────
if [ "$WITH_COVERAGE" = "true" ]; then
  BUILD_NAME="${BUILD_TYPE}_cov"
else
  BUILD_NAME="${BUILD_TYPE}"
fi

SRC_DIR="${BASE_DIR}/src"
BUILD_DIR="${BASE_DIR}/build/${BUILD_NAME}"
OUT_DIR="${BASE_DIR}/out/${BUILD_NAME}"
RESOURCES_DIR="${BASE_DIR}/resources"

# ── Platform detection ────────────────────────────────────────────────────
if [ "$(uname)" = "Darwin" ]; then
  NPROC=$(sysctl -n hw.logicalcpu)
else
  NPROC=$(nproc --all)
fi

# ── Banner ────────────────────────────────────────────────────────────────
echo ""
echo "===================================================================="
echo "  MNE-CPP  Build & Test"
echo "===================================================================="
echo "  Build type  : $BUILD_TYPE"
echo "  Build name  : $BUILD_NAME"
echo "  Coverage    : $WITH_COVERAGE"
echo "  Parallelism : $NPROC cores"
echo "  Build dir   : $BUILD_DIR"
echo "  Output dir  : $OUT_DIR"
echo "===================================================================="
echo ""

# ======================================================================
# STEP 1 — Ensure test data
# ======================================================================
echo "── [1/6] Checking test data ──────────────────────────────────────"

# mne-cpp-test-data
if [ ! -d "${RESOURCES_DIR}/data/mne-cpp-test-data/.git" ]; then
  echo "  Cloning mne-cpp-test-data …"
  git clone --depth 1 https://github.com/mne-tools/mne-cpp-test-data.git \
    "${RESOURCES_DIR}/data/mne-cpp-test-data"
else
  echo "  mne-cpp-test-data already present — pulling latest …"
  git -C "${RESOURCES_DIR}/data/mne-cpp-test-data" pull --ff-only || true
fi

# MNE sample data (try mne-python first, fall back to curl)
MNE_DATA="${HOME}/mne_data"
if [ ! -d "${MNE_DATA}/MNE-sample-data" ]; then
  echo "  Downloading MNE sample data …"
  if command -v python3 &>/dev/null && python3 -c "import mne" &>/dev/null; then
    python3 -c "import mne; mne.datasets.sample.data_path()"
  elif command -v python &>/dev/null && python -c "import mne" &>/dev/null; then
    python -c "import mne; mne.datasets.sample.data_path()"
  else
    echo "  mne-python not found — downloading via curl …"
    mkdir -p "${MNE_DATA}"
    curl -L -o "${MNE_DATA}/MNE-sample-data.tar.gz" \
      "https://files.osf.io/v1/resources/rxvq7/providers/osfstorage/59c0e26f9ad5a1025c4ab159?action=download&direct&version=6"
    tar -xzf "${MNE_DATA}/MNE-sample-data.tar.gz" -C "${MNE_DATA}"
    rm -f "${MNE_DATA}/MNE-sample-data.tar.gz"
  fi
else
  echo "  MNE sample data already present at ${MNE_DATA}/MNE-sample-data"
fi
echo ""

# ======================================================================
# STEP 2 — Clean previous build
# ======================================================================
echo "── [2/6] Clean build ─────────────────────────────────────────────"
if [ -d "$BUILD_DIR" ]; then
  echo "  Removing ${BUILD_DIR} …"
  rm -rf "$BUILD_DIR"
fi
if [ -d "$OUT_DIR" ]; then
  echo "  Removing ${OUT_DIR} …"
  rm -rf "$OUT_DIR"
fi
echo "  Done."
echo ""

# ======================================================================
# STEP 3 — Configure CMake
# ======================================================================
echo "── [3/6] Configuring CMake ───────────────────────────────────────"

CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
CMAKE_FLAGS="${CMAKE_FLAGS} -DBUILD_TESTS=ON"
CMAKE_FLAGS="${CMAKE_FLAGS} -DBINARY_OUTPUT_DIRECTORY=${OUT_DIR}"

if [ "$WITH_COVERAGE" = "true" ]; then
  CMAKE_FLAGS="${CMAKE_FLAGS} -DWITH_CODE_COV=ON"
fi

# shellcheck disable=SC2086
cmake -B "$BUILD_DIR" -S "$SRC_DIR" ${CMAKE_FLAGS} ${EXTRA_CMAKE_FLAGS}
echo ""

# ======================================================================
# STEP 4 — Build
# ======================================================================
echo "── [4/6] Building (${NPROC} cores) ──────────────────────────────"
cmake --build "$BUILD_DIR" --parallel "$NPROC"

# Copy compile_commands.json to root for LSP
cp -f "${BUILD_DIR}/compile_commands.json" "${BASE_DIR}/" 2>/dev/null || true
echo ""

# ======================================================================
# STEP 5 — Run tests
# ======================================================================
echo "── [5/6] Running tests ───────────────────────────────────────────"

export QT_QPA_PLATFORM=offscreen
export MNE_DATA="${MNE_DATA}"

if [ "$WITH_COVERAGE" = "true" ]; then
  export QTEST_FUNCTION_TIMEOUT=1800000   # 30 min per test function
  "${SCRIPT_DIR}/test_all.bat" verbose withCoverage "build-name=${BUILD_NAME}"
else
  export QTEST_FUNCTION_TIMEOUT=900000    # 15 min per test function
  "${SCRIPT_DIR}/test_all.bat" verbose "build-name=${BUILD_NAME}"
fi

TEST_EXIT=$?
echo ""

# ======================================================================
# STEP 6 — Coverage report (only when enabled)
# ======================================================================
if [ "$WITH_COVERAGE" = "true" ]; then
  echo "── [6/6] Generating coverage report ──────────────────────────────"

  if command -v fastcov &>/dev/null; then
    echo "  Using fastcov …"
    fastcov -d "$BUILD_DIR" -o "${BUILD_DIR}/coverage.json"
    fastcov -C "${BUILD_DIR}/coverage.json" --lcov -o "${BUILD_DIR}/coverage.info"
    fastcov -C "${BUILD_DIR}/coverage.json" --lcov -o "${BUILD_DIR}/coverage_filtered.info" \
      --include src/libraries/ \
      --exclude /usr Qt eigen

    # Extract summary
    TOTAL_LINES=$(grep -c "^DA:" "${BUILD_DIR}/coverage_filtered.info" 2>/dev/null || echo "0")
    HIT_LINES=$(grep "^DA:" "${BUILD_DIR}/coverage_filtered.info" | awk -F, '$2 > 0' | wc -l 2>/dev/null || echo "0")
    TOTAL_LINES=$(echo "$TOTAL_LINES" | tr -d ' ')
    HIT_LINES=$(echo "$HIT_LINES" | tr -d ' ')
    if [ "$TOTAL_LINES" -gt 0 ]; then
      COVERAGE=$(awk "BEGIN {printf \"%.2f\", $HIT_LINES * 100.0 / $TOTAL_LINES}")
    else
      COVERAGE="0.00"
    fi

    echo ""
    echo "  ════════════════════════════════════════════════"
    echo "  Coverage Summary (libraries only)"
    echo "  ════════════════════════════════════════════════"
    echo "  Lines hit  : $HIT_LINES / $TOTAL_LINES"
    echo "  Coverage   : ${COVERAGE}%"
    echo "  ════════════════════════════════════════════════"
    echo ""
    echo "  Reports:"
    echo "    Full   : ${BUILD_DIR}/coverage.info"
    echo "    Filtered: ${BUILD_DIR}/coverage_filtered.info"
    echo ""

    # HTML report if genhtml is available
    if command -v genhtml &>/dev/null; then
      COVERAGE_HTML="${BUILD_DIR}/coverage_html"
      genhtml "${BUILD_DIR}/coverage_filtered.info" \
        --output-directory "$COVERAGE_HTML" --quiet
      echo "  HTML report: ${COVERAGE_HTML}/index.html"
      echo ""
    fi

  elif command -v lcov &>/dev/null; then
    echo "  Using lcov (slower — consider installing fastcov) …"
    lcov --capture --directory "$BUILD_DIR" \
      --output-file "${BUILD_DIR}/coverage.info" --quiet
    lcov --remove "${BUILD_DIR}/coverage.info" \
      '/usr/*' '*/Qt/*' '*/eigen/*' '*/external/*' '*/testframes/*' '*/examples/*' '*/applications/*' \
      --output-file "${BUILD_DIR}/coverage_filtered.info" --quiet

    TOTAL_LINES=$(grep -c "^DA:" "${BUILD_DIR}/coverage_filtered.info" 2>/dev/null || echo "0")
    HIT_LINES=$(grep "^DA:" "${BUILD_DIR}/coverage_filtered.info" | awk -F, '$2 > 0' | wc -l 2>/dev/null || echo "0")
    TOTAL_LINES=$(echo "$TOTAL_LINES" | tr -d ' ')
    HIT_LINES=$(echo "$HIT_LINES" | tr -d ' ')
    if [ "$TOTAL_LINES" -gt 0 ]; then
      COVERAGE=$(awk "BEGIN {printf \"%.2f\", $HIT_LINES * 100.0 / $TOTAL_LINES}")
    else
      COVERAGE="0.00"
    fi

    echo ""
    echo "  ════════════════════════════════════════════════"
    echo "  Coverage Summary (libraries only)"
    echo "  ════════════════════════════════════════════════"
    echo "  Lines hit  : $HIT_LINES / $TOTAL_LINES"
    echo "  Coverage   : ${COVERAGE}%"
    echo "  ════════════════════════════════════════════════"
    echo ""
    echo "  Report: ${BUILD_DIR}/coverage_filtered.info"
    echo ""

    if command -v genhtml &>/dev/null; then
      COVERAGE_HTML="${BUILD_DIR}/coverage_html"
      genhtml "${BUILD_DIR}/coverage_filtered.info" \
        --output-directory "$COVERAGE_HTML" --quiet
      echo "  HTML report: ${COVERAGE_HTML}/index.html"
      echo ""
    fi

  else
    echo "  ⚠  Neither fastcov nor lcov found."
    echo "     Install one of them for coverage reports:"
    echo "       pip install fastcov"
    echo "       brew install lcov        # macOS"
    echo "       apt install lcov         # Linux"
    echo ""
    echo "  Raw gcov data is still available in ${BUILD_DIR}/"
    echo "  (.gcno / .gcda files)"
    echo ""
  fi
else
  echo "── [6/6] Coverage skipped (pass 'coverage' to enable) ────────────"
fi

echo ""
echo "===================================================================="
if [ "$TEST_EXIT" -eq 0 ]; then
  echo "  ✅  All done — tests passed."
else
  echo "  ❌  Done — $TEST_EXIT test(s) failed."
fi
echo "===================================================================="

exit "$TEST_EXIT"
