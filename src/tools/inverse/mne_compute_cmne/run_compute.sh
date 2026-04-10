#!/bin/bash
#
# run_compute.sh — Convenience launcher for mne_compute_cmne (compute mode)
#
# Computes dSPM + CMNE source estimates from MNE sample data and writes
# STC files.  Uses the MNE sample dataset by default.
#
# Usage:
#   ./run_compute.sh                     # moving-average correction (no ONNX)
#   ./run_compute.sh --onnx model.onnx   # LSTM correction with trained model
#

set -euo pipefail

# ── Default data paths (MNE sample dataset) ─────────────────────────────
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
SampleDir="${MneSampleDataPath}/MEG/sample"
SubjectDir="${MneSampleDataPath}/subjects"

FwdFile="${SampleDir}/sample_audvis-meg-eeg-oct-6-fwd.fif"
CovFile="${SampleDir}/sample_audvis-cov.fif"
EvokedFile="${SampleDir}/sample_audvis-ave.fif"

# ── Resolve build directory ─────────────────────────────────────────────
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OUT_DIR="$SCRIPT_DIR/../../../../out"

BuildRoot=""
BuildPath=""

select_build_root() {
    local candidate_root="$1"
    local candidate_bin="$candidate_root/bin"
    local candidate_lib="$candidate_root/lib"
    local bin_path="$candidate_bin/mne_compute_cmne"

    if [ -f "$bin_path" ] && [ -f "$candidate_lib/libmne_inv.dylib" -o -f "$candidate_lib/libmne_inv.so" ]; then
        BuildRoot="$candidate_root"
        BuildPath="$bin_path"
        return 0
    fi

    return 1
}

for candidate in "$OUT_DIR/Release" "$OUT_DIR/Coverage" "$OUT_DIR/Debug"; do
    if select_build_root "$candidate"; then
        break
    fi
done

if [ -z "$BuildPath" ]; then
    echo "Error: Could not locate mne_compute_cmne binary."
    echo "Checked:"
    echo "  $OUT_DIR/Release/bin"
    echo "  $OUT_DIR/Coverage/bin"
    echo "  $OUT_DIR/Debug/bin"
    echo
    echo "Build the tool first:"
    echo "  cmake --build build --target mne_compute_cmne"
    exit 1
fi

LIB_DIR="$BuildRoot/lib"

# ── Set library paths ───────────────────────────────────────────────────
if [[ "$OSTYPE" == "darwin"* ]]; then
    QT_BASE="${QT_BASE:-$HOME/Qt/6.11.0/macos}"

    unset QT_PLUGIN_PATH 2>/dev/null || true
    unset QT_QPA_PLATFORM_PLUGIN_PATH 2>/dev/null || true

    if [ -d "$QT_BASE/plugins" ]; then
        export QT_PLUGIN_PATH="$QT_BASE/plugins"
    fi

    # Include ONNX Runtime if available
    ORT_LIB="$SCRIPT_DIR/../../../../src/external/onnxruntime/lib"
    EXTRA_LIB=""
    [ -d "$ORT_LIB" ] && EXTRA_LIB=":$ORT_LIB"

    if [ -d "$QT_BASE/lib" ]; then
        export DYLD_FRAMEWORK_PATH="$QT_BASE/lib"
        export DYLD_LIBRARY_PATH="$LIB_DIR:$QT_BASE/lib${EXTRA_LIB}${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    else
        export DYLD_LIBRARY_PATH="$LIB_DIR${EXTRA_LIB}${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    fi
else
    QT_BASE="${QT_BASE:-$HOME/Qt/6.11.0/gcc_64}"
    if [ -d "$QT_BASE/plugins" ]; then
        export QT_PLUGIN_PATH="$QT_BASE/plugins"
    fi

    ORT_LIB="$SCRIPT_DIR/../../../../src/external/onnxruntime/lib"
    EXTRA_LIB=""
    [ -d "$ORT_LIB" ] && EXTRA_LIB=":$ORT_LIB"

    export LD_LIBRARY_PATH="$LIB_DIR${EXTRA_LIB}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

# ── Verify sample data ──────────────────────────────────────────────────
for f in "$FwdFile" "$CovFile" "$EvokedFile"; do
    if [ ! -f "$f" ]; then
        echo "Error: Required file not found: $f"
        echo "Download MNE sample data or set MNE_DATA_PATH."
        exit 1
    fi
done

# ── Output location ─────────────────────────────────────────────────────
OUTPUT_DIR="${SCRIPT_DIR}/output"
mkdir -p "$OUTPUT_DIR"
OUT_PREFIX="${OUTPUT_DIR}/sample_audvis"

# ── Run ──────────────────────────────────────────────────────────────────
echo "=== mne_compute_cmne — Compute Mode ==="
echo "  Forward   : $FwdFile"
echo "  Covariance: $CovFile"
echo "  Evoked    : $EvokedFile"
echo "  Output    : ${OUT_PREFIX}-dspm.stc / ${OUT_PREFIX}-cmne.stc"
echo

CMD_ARGS=(
    --mode compute
    --fwd "$FwdFile"
    --cov "$CovFile"
    --evoked "$EvokedFile"
    --out "$OUT_PREFIX"
    --snr 3.0
    --method dSPM
    --look-back 80
)

# Forward any extra arguments (e.g. --onnx model.onnx)
CMD_ARGS+=("$@")

echo "Running: $BuildPath ${CMD_ARGS[*]}"
echo

"$BuildPath" "${CMD_ARGS[@]}"
