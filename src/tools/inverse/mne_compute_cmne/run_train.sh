#!/bin/bash
#
# run_train.sh — Convenience launcher for mne_compute_cmne (train / finetune mode)
#
# Trains the CMNE LSTM model from MNE sample data and exports to ONNX.
# The script automatically creates a Python venv and installs dependencies
# from the bundled pyproject.toml on first run.
#
# Usage:
#   ./run_train.sh                                  # Train from scratch
#   ./run_train.sh --finetune output/cmne_lstm.onnx # Fine-tune existing model
#   ./run_train.sh --train-epochs 100 --lr 0.0005   # Override hyper-parameters
#

set -euo pipefail

# ── Default data paths (MNE sample dataset) ─────────────────────────────
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
SampleDir="${MneSampleDataPath}/MEG/sample"

FwdFile="${SampleDir}/sample_audvis-meg-eeg-oct-6-fwd.fif"
CovFile="${SampleDir}/sample_audvis-cov.fif"
RawFile="${SampleDir}/sample_audvis_raw.fif"
EventsFile="${SampleDir}/sample_audvis_raw-eve.fif"

# Epochs are generated on the fly from raw + events (MNE sample data
# ships with the averaged evoked file sample_audvis-ave.fif, not epochs).
# The generated file is stored in the output directory.

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
for f in "$FwdFile" "$CovFile" "$RawFile"; do
    if [ ! -f "$f" ]; then
        echo "Error: Required file not found: $f"
        echo "Download MNE sample data or set MNE_DATA_PATH."
        exit 1
    fi
done

# ── Output location ─────────────────────────────────────────────────────
OUTPUT_DIR="${SCRIPT_DIR}/output"
mkdir -p "$OUTPUT_DIR"
ONNX_OUT="${OUTPUT_DIR}/cmne_lstm.onnx"

# ── Auto-generate epochs from raw data ──────────────────────────────────
# MNE sample data ships sample_audvis-ave.fif (averaged evoked), not
# epochs.  We create epochs from the raw file + events on the fly.
EpochsFile="${OUTPUT_DIR}/sample_audvis-epo.fif"

if [ ! -f "$EpochsFile" ]; then
    echo "Generating epochs from raw data (first run only) …"

    PYTHON_CMD="${PYTHON_CMD:-python3}"
    if ! command -v "$PYTHON_CMD" &>/dev/null; then
        echo "Error: Python not found ('$PYTHON_CMD'). Install Python or set PYTHON_CMD."
        exit 1
    fi

    "$PYTHON_CMD" - "$RawFile" "$EventsFile" "$EpochsFile" <<'PYEOF'
import sys
import mne

raw_path, events_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]

print(f"  Raw   : {raw_path}")
raw = mne.io.read_raw_fif(raw_path, preload=True, verbose=False)
raw.filter(1.0, 40.0, verbose=False)

if events_path and __import__('os').path.isfile(events_path):
    print(f"  Events: {events_path}")
    events = mne.read_events(events_path)
else:
    print("  Events: auto-detected from raw (STI 014)")
    events = mne.find_events(raw, stim_channel="STI 014", verbose=False)

event_id = {"aud/l": 1, "aud/r": 2, "vis/l": 3, "vis/r": 4}
print(f"  {len(events)} events found, creating epochs (tmin=-0.2, tmax=0.5) …")

epochs = mne.Epochs(raw, events, event_id=event_id,
                    tmin=-0.2, tmax=0.5, baseline=(None, 0),
                    preload=True, verbose=False)
epochs.drop_bad(verbose=False)
print(f"  {len(epochs)} epochs after rejection")
epochs.save(out_path, overwrite=True, verbose=False)
print(f"  Saved: {out_path}")
PYEOF

    if [ $? -ne 0 ]; then
        echo "Error: Failed to generate epochs. Is MNE-Python installed?"
        echo "  pip install mne"
        exit 1
    fi
    echo
else
    echo "Using cached epochs: $EpochsFile"
fi

# ── Detect fine-tune mode from arguments ─────────────────────────────────
MODE="train"
for arg in "$@"; do
    if [ "$arg" = "--finetune" ]; then
        MODE="finetune"
        break
    fi
done

# ── Run ──────────────────────────────────────────────────────────────────
echo "=== mne_compute_cmne — $(echo "$MODE" | awk '{print toupper(substr($0,1,1)) substr($0,2)}') Mode ==="
echo "  Forward   : $FwdFile"
echo "  Covariance: $CovFile"
echo "  Epochs    : $EpochsFile"
echo "  ONNX out  : $ONNX_OUT"
echo

CMD_ARGS=(
    --mode "$MODE"
    --fwd "$FwdFile"
    --cov "$CovFile"
    --epochs "$EpochsFile"
    --onnx-out "$ONNX_OUT"
    --snr 3.0
    --method dSPM
    --look-back 40
    --hidden 256
    --layers 1
    --train-epochs 50
    --lr 0.001
    --batch 64
)

# Forward any extra arguments (e.g. --finetune model.onnx, --train-epochs 100)
CMD_ARGS+=("$@")

echo "Running: $BuildPath ${CMD_ARGS[*]}"
echo

"$BuildPath" "${CMD_ARGS[@]}"

# ── Post-training summary ────────────────────────────────────────────────
echo
if [ -f "$ONNX_OUT" ]; then
    echo "=== Training Complete ==="
    echo "  ONNX model  : $ONNX_OUT ($(du -h "$ONNX_OUT" | cut -f1))"
    PT_OUT="${ONNX_OUT%.onnx}.pt"
    [ -f "$PT_OUT" ] && echo "  PT checkpoint: $PT_OUT ($(du -h "$PT_OUT" | cut -f1))"
    echo
    echo "To compute CMNE STCs with this model, run:"
    echo "  ./run_compute.sh --onnx $ONNX_OUT"
    echo
    echo "To fine-tune further:"
    echo "  ./run_train.sh --finetune $ONNX_OUT --train-epochs 20"
fi
