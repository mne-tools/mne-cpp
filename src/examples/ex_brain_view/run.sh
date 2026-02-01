#!/bin/bash

# Default values
SubjectPath="/Users/christoph.dinh/mne_data/MNE-sample-data/subjects"
Subject="sample"
Hemi=0

# Construct the full path to the executable
BuildPath="/Users/christoph.dinh/Programming/mne-cpp/out/Release/examples/ex_brain_view.app/Contents/MacOS/ex_brain_view"

# Kill any existing instances
pkill -f ex_brain_view || true

# Check if executable exists
if [ ! -f "$BuildPath" ]; then
    echo "Executable not found at $BuildPath"
    echo "Please build the project first."
    exit 1
fi

echo "launching ex_brain_view..."
"$BuildPath" --subjectPath "$SubjectPath" --subject "$Subject" --hemi "$Hemi"
