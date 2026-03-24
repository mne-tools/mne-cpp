#!/bin/bash

set -euo pipefail

# Default values
SubjectPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data/subjects}"
Subject="sample"
Hemi=0

# Default data paths
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
SampleDir="${MneSampleDataPath}/MEG/sample"
ProcessedDir="${SampleDir}/processed"
DigitizerFile="${SampleDir}/sample_audvis_raw.fif"
TransFile="${SampleDir}/all-trans.fif"
EvokedFile="${SampleDir}/sample_audvis-ave.fif"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OUT_DIR="$SCRIPT_DIR/../../../out"

BuildRoot=""
BuildPath=""

select_build_root() {
    local candidate_root="$1"
    local candidate_bin="$candidate_root/bin"
    local candidate_lib="$candidate_root/lib"
    local app_path="$candidate_bin/mne_inspect.app/Contents/MacOS/mne_inspect"
    local bin_path="$candidate_bin/mne_inspect"

    if [ -f "$app_path" ] && [ -f "$candidate_lib/libmne_disp3D.dylib" ]; then
        BuildRoot="$candidate_root"
        BuildPath="$app_path"
        return 0
    fi

    if [ -f "$bin_path" ] && [ -f "$candidate_lib/libmne_disp3D.dylib" ]; then
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
    echo "Error: Could not locate a runnable mne_inspect build."
    echo "Checked for executable + libmne_disp3D.dylib in:"
    echo "  $OUT_DIR/Release"
    echo "  $OUT_DIR/Coverage"
    echo "  $OUT_DIR/Debug"
    echo
    echo "Build or rebuild the missing runtime library, for example:"
    echo "  cmake --build build --config Release --target mne_inspect"
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

if pgrep -x "mne_inspect" >/dev/null 2>&1; then
    pkill -x "mne_inspect" || true
fi

BemFile="${SubjectPath}/${Subject}/bem/sample-5120-5120-5120-bem.fif"
SrcSpaceFile="${SubjectPath}/${Subject}/bem/sample-oct-6-orig-src.fif"
AtlasFile="${SubjectPath}/${Subject}/label/lh.aparc.annot"

echo "Launching MNE Inspect from $BuildPath..."

declare -a STC_ARGS=()
if [ -d "$ProcessedDir" ]; then
    for stc in "$ProcessedDir"/*-lh.stc; do
        [ -f "$stc" ] && STC_ARGS+=(--stc "$stc")
    done
fi
if [ ${#STC_ARGS[@]} -eq 0 ]; then
    FallbackStc="${SampleDir}/sample_audvis-meg-eeg-lh.stc"
    [ -f "$FallbackStc" ] && STC_ARGS=(--stc "$FallbackStc")
fi

"$BuildPath" --subjectPath "$SubjectPath" --subject "$Subject" --hemi "$Hemi" --bem "$BemFile" \
    "${STC_ARGS[@]}" --digitizer "$DigitizerFile" --trans "$TransFile" --srcSpace "$SrcSpaceFile" \
    --atlas "$AtlasFile" --evoked "$EvokedFile"
