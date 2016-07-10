#!/bin/bash
# ===startdir=== %WORKSPACE%/mne-cpp/..
# %0 Build Number
# arg0=%0

echo Starting MNE-CPP Mac Build

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

QT_BIN_DIR='/Users/Shared/Jenkins/Qt5.7.0/5.7/clang_64/bin'
QT_LIB_DIR='/Users/Shared/Jenkins/Qt5.7.0/5.7/clang_64/lib'

TANGIBLES=(mne_x mne_browse_raw_qt mne_analyze_qt)

PATH=$QT_BIN_DIR:$PATH
export PATH

DYLD_LIBRARY_PATH="/Users/Shared/Jenkins/Home/jobs/MNE-CPP/workspace/mne-cpp/lib":$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH
DYLD_FALLBACK_LIBRARY_PATH="/Users/Shared/Jenkins/Home/jobs/MNE-CPP/workspace/mne-cpp/lib"
export DYLD_FALLBACK_LIBRARY_PATH

# === Clean Up ===
n_elements=${#TANGIBLES[@]}
for ((i = 0; i < n_elements; i++)); do
    tangibleapp="../mne-cpp/bin/${TANGIBLES[i]}.app"
    tangibledmgbin="../mne-cpp/bin/${TANGIBLES[i]}.dmg"
    tangibledmg="../${TANGIBLES[i]}-master.dmg"
    rm -rf $tangibleapp
	rm $tangibledmgbin
	rm $tangibledmg
done

# === Build ===
qmake ../mne-cpp/mne-cpp.pro -r
make clean
make -j4

# === Deployment ===
for ((i = 0; i < n_elements; i++)); do
    fixfile="../mne-cpp/bin/${TANGIBLES[i]}.app/Contents/MacOS/${TANGIBLES[i]}"
    destdir="../mne-cpp/bin/${TANGIBLES[i]}.app/Contents/Frameworks/"

	/usr/local/bin/dylibbundler -od -b -x $fixfile -d $destdir -p @executable_path/../Frameworks/
	
    tangible="../mne-cpp/bin/${TANGIBLES[i]}.app"
    macdeployqt $tangible -dmg
done

# === Copy Tangibles ===
cd ..
for ((i = 0; i < n_elements; i++)); do
	cp "./mne-cpp/bin/${TANGIBLES[i]}.dmg" "./"
	mv "./${TANGIBLES[i]}.dmg" "./${TANGIBLES[i]}-master.dmg"
done
