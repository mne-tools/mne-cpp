#!/bin/bash
#
# run.sh - Run mne_compute_raw_inverse with MNE sample data
#
# Computes the full source space dSPM inverse solution of
# sample_audvis-ave.fif using the MEG+EEG inverse operator
# for all evoked data sets (Left Auditory, Right Auditory,
# Left visual, Right visual).
#
# Output is written to <sample-data>/MEG/sample/processed/
#
# Parameters:
#   SNR     = 3      (lambda2 = 1/9)
#   method  = dSPM   (--spm)
#   sets    = all    (default, no --set)
#   baseline: -200 to 0 ms  (matching mne-python baseline=(None, 0))
#

set -e

# Default MNE sample data path
MneSampleDataPath="${MNE_DATA_PATH:-$HOME/mne_data/MNE-sample-data}"
SampleDir="${MneSampleDataPath}/MEG/sample"

# Input files
EvokedFile="${SampleDir}/sample_audvis-ave.fif"
InvFile="${SampleDir}/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif"

# Output directory
OutDir="${SampleDir}/processed"

# Locate executable relative to this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BuildPath="${SCRIPT_DIR}/../../../out/Release/apps/mne_compute_raw_inverse"

# Check if executable exists
if [ ! -f "$BuildPath" ]; then
    echo "Error: Could not locate mne_compute_raw_inverse executable at:"
    echo "  $BuildPath"
    echo "Please build the project first."
    exit 1
fi

# Check if sample data exists
if [ ! -f "$EvokedFile" ]; then
    echo "Error: Sample data not found at:"
    echo "  $EvokedFile"
    echo "Set MNE_DATA_PATH or download the MNE sample dataset to ~/mne_data/"
    exit 1
fi

if [ ! -f "$InvFile" ]; then
    echo "Error: Inverse operator file not found at:"
    echo "  $InvFile"
    exit 1
fi

# Create output directory
mkdir -p "$OutDir"

echo "=============================================="
echo " mne_compute_raw_inverse - MNE Sample Data"
echo "=============================================="
echo ""
echo "Evoked file  : $EvokedFile"
echo "Inverse file : $InvFile"
echo "Output dir   : $OutDir"
echo ""

# Run full source space dSPM inverse for all evoked sets
# Parameters: SNR=3, dSPM, all sets, baseline -200..0 ms
echo "--- Computing full source space dSPM inverse (all sets) ---"
"$BuildPath" \
    --in "$EvokedFile" \
    --inv "$InvFile" \
    --snr 3 \
    --spm \
    --bmin -200 \
    --bmax 0 \
    --out "$OutDir/sample_audvis-ave-spm"

echo ""

# Verify output files were created
STC_COUNT=$(find "$OutDir" -name "sample_audvis-ave-spm-*.stc" | wc -l)
if [ "$STC_COUNT" -eq 0 ]; then
    echo "Error: No output STC files created."
    ls -la "$OutDir/"
    exit 1
fi

echo "Output files:"
ls -lh "$OutDir"/sample_audvis-ave-spm-*.stc

echo ""
echo "Done."
