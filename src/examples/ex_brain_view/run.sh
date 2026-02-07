#!/bin/bash

# Default values
SubjectPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data/subjects}"
Subject="sample"
Hemi=0

# Default data paths
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
StcFile="${MneSampleDataPath}/MEG/sample/sample_audvis-meg-eeg-lh.stc"
DigitizerFile="${MneSampleDataPath}/MEG/sample/sample_audvis_raw.fif"
TransFile="${MneSampleDataPath}/MEG/sample/all-trans.fif"

# Default build path (assuming run from project root or similar structure)
# Try to find the executable relative to the script location
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
# Assuming standard build output structure relative to src/examples/ex_brain_view
BuildPath="$SCRIPT_DIR/../../../out/Release/examples/ex_brain_view"

# Kill any existing instances
pkill -f ex_brain_view || true

# Check if executable exists


if [ ! -f "$BuildPath" ]; then
     echo "Error: Could not locate ex_brain_view executable."
     exit 1
fi

BemFile="${SubjectPath}/${Subject}/bem/sample-5120-5120-5120-bem.fif"

echo "launching ex_brain_view from $BuildPath..."
"$BuildPath" --subjectPath "$SubjectPath" --subject "$Subject" --hemi "$Hemi" --bem "$BemFile" \
    --stc "$StcFile" --digitizer "$DigitizerFile" --trans "$TransFile"
