#!/bin/bash

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

# Default build path (assuming run from project root or similar structure)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BuildPath="$SCRIPT_DIR/../../../out/Release/apps/mne_inspect"

# Kill any existing instances
pkill -f mne_inspect || true

# Check if executable exists
if [ ! -f "$BuildPath" ]; then
    # Try macOS app bundle
    BuildPath="$SCRIPT_DIR/../../../out/Release/apps/mne_inspect.app/Contents/MacOS/mne_inspect"
fi

if [ ! -f "$BuildPath" ]; then
     echo "Error: Could not locate mne_inspect executable."
     exit 1
fi

BemFile="${SubjectPath}/${Subject}/bem/sample-5120-5120-5120-bem.fif"
SrcSpaceFile="${SubjectPath}/${Subject}/bem/sample-oct-6-orig-src.fif"
AtlasFile="${SubjectPath}/${Subject}/label/lh.aparc.annot"

echo "launching MNE Inspect from $BuildPath..."

# Build --stc arguments: load all *-lh.stc files from the processed/ directory
STC_ARGS=""
if [ -d "$ProcessedDir" ]; then
    for stc in "$ProcessedDir"/*-lh.stc; do
        [ -f "$stc" ] && STC_ARGS="$STC_ARGS --stc $stc"
    done
fi
# Fallback to the original sample STC if no processed STCs exist
if [ -z "$STC_ARGS" ]; then
    FallbackStc="${SampleDir}/sample_audvis-meg-eeg-lh.stc"
    [ -f "$FallbackStc" ] && STC_ARGS="--stc $FallbackStc"
fi

"$BuildPath" --subjectPath "$SubjectPath" --subject "$Subject" --hemi "$Hemi" --bem "$BemFile" \
    $STC_ARGS --digitizer "$DigitizerFile" --trans "$TransFile" --srcSpace "$SrcSpaceFile" \
    --atlas "$AtlasFile" --evoked "$EvokedFile"
