#!/bin/bash

# Default values
SubjectPath="/Users/christoph.dinh/mne_data/MNE-sample-data/subjects"
Subject="sample"
Hemi=0

# Construct the full path to the executable
BuildPath="/Users/christoph.dinh/Programming/mne-cpp/out/Release/examples/ex_brain_view"

# Kill any existing instances
pkill -f ex_brain_view || true

# Check if executable exists
# Default values
BemFile="/Users/christoph.dinh/mne_data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif"
# Check arguments if user overrides
# ... (simplified for now)

echo "launching ex_brain_view..."
"$BuildPath" --subjectPath "$SubjectPath" --subject "$Subject" --hemi "$Hemi" --bem "$BemFile"
