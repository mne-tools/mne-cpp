#!/bin/bash

set -euo pipefail

# Default data paths
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
SampleDir="${MneSampleDataPath}/MEG/sample"
RawFile="${SampleDir}/sample_audvis_raw.fif"
EventsFile="${SampleDir}/sample_audvis-eve.fif"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OUT_DIR="$SCRIPT_DIR/../../../out"

BuildRoot=""
BuildPath=""

select_build_root() {
    local candidate_root="$1"
    local candidate_bin="$candidate_root/bin"
    local candidate_lib="$candidate_root/lib"
    local app_path="$candidate_bin/mne_browse.app/Contents/MacOS/mne_browse"
    local bin_path="$candidate_bin/mne_browse"

    if [ -f "$app_path" ] && [ -f "$candidate_lib/libmne_disp.dylib" ]; then
        BuildRoot="$candidate_root"
        BuildPath="$app_path"
        return 0
    fi

    if [ -f "$bin_path" ] && [ -f "$candidate_lib/libmne_disp.dylib" ]; then
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
    echo "Error: Could not locate a runnable mne_browse build."
    echo "Checked for executable + libmne_disp.dylib in:"
    echo "  $OUT_DIR/Release"
    echo "  $OUT_DIR/Coverage"
    echo "  $OUT_DIR/Debug"
    echo
    echo "Build the application first, for example:"
    echo "  cmake --build build --config Release --target mne_browse"
    exit 1
fi

BIN_DIR="$BuildRoot/bin"
LIB_DIR="$BuildRoot/lib"

if [[ "$OSTYPE" == "darwin"* ]]; then
    QT_BASE="${QT_BASE:-$HOME/Qt/6.10.2/macos}"

    unset QT_PLUGIN_PATH
    unset QT_QPA_PLATFORM_PLUGIN_PATH

    if [ -d "$QT_BASE/plugins" ]; then
        export QT_PLUGIN_PATH="$QT_BASE/plugins"
        export QT_QPA_PLATFORM_PLUGIN_PATH="$QT_BASE/plugins/platforms"
    fi

    if [ -d "$QT_BASE/lib" ]; then
        export DYLD_FRAMEWORK_PATH="$QT_BASE/lib"
        export DYLD_LIBRARY_PATH="$LIB_DIR:$QT_BASE/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    else
        export DYLD_LIBRARY_PATH="$LIB_DIR${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    fi
else
    QT_BASE="${QT_BASE:-$HOME/Qt/6.10.2/gcc_64}"
    if [ -d "$QT_BASE/plugins" ]; then
        export QT_PLUGIN_PATH="$QT_BASE/plugins"
    fi
    export LD_LIBRARY_PATH="$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

if pgrep -x "mne_browse" >/dev/null 2>&1; then
    pkill -x "mne_browse" || true
fi

if [ ! -f "$RawFile" ]; then
    echo "Error: Sample data file not found: $RawFile"
    echo "Download MNE sample data to $MneSampleDataPath or set MNE_DATA_PATH."
    exit 1
fi

echo "Launching MNE Browse from $BuildPath..."

EXTRA_ARGS=()
[ -f "$EventsFile" ] && EXTRA_ARGS+=(--events "$EventsFile")

"$BuildPath" --raw "$RawFile" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
