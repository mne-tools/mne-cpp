#!/bin/bash

###
# It is recommended you run this script while in an empty directory.
# The script will download and build the necessary components, and 
# reuse them if they are already present. 
###

argc=$#
argv=("$@")

## User editable through flags
SOURCE_REPO=""
EMSDK_VERSION="latest"
QT_VERSION="5.15"

## Stop at first error
set -e

for (( j=0; j<argc; j++)); do
    if [ "${argv[j]}" == "--source" ] || [ "${argv[j]}" == "-s" ]; then
        SOURCE_REPO="${argv[j+1]}" 
    elif [ "${argv[j]}" == "--emsdk" ] || [ "${argv[j]}" == "-e" ]; then
        EMSDK_VERSION="${argv[j+1]}" 
    elif [ "${argv[j]}" == "--qt" ] || [ "${argv[j]}" == "-q" ]; then
        QT_VERSION="${argv[j+1]}" 
    fi
done

# echo "${SOURCE_REPO}"
# echo "${EMSDK_VERSION}"
# echo "${QT_VERSION}"

if [ "${SOURCE_REPO}" ]; then
	echo "Source repo set to ${SOURCE_REPO}"
else
	echo "No source specified."
	echo "Please set a source with the '--source' flag."
	exit 1
fi

## emsdk
echo "Setting up emsdk ${EMSDK_VERSION}..."
if [ -d "emsdk" ]; then
	echo "Found emsdk folder."
else
	echo "emsdk folder not found. Fetching repo..."
	git clone https://github.com/emscripten-core/emsdk.git
fi
cd emsdk
git pull
./emsdk install ${EMSDK_VERSION}
./emsdk activate ${EMSDK_VERSION}
source ./emsdk_env.sh
cd ..
echo "emsdk set up."

## Qt
echo "Setting up Qt ${QT_VERSION}..."
if [ -d "Qt5_binaries" ]; then
	echo "Found Qt5_binaries folder."
else
	echo "Qt5_binaries folder not found. Fetching repo and building..."
	git clone https://code.qt.io/qt/qt5.git -b ${QT_VERSION}
	cd qt5
	./init-repository -f --module-subset=qtbase,qtcharts,qtsvg
	./configure -xplatform wasm-emscripten -feature-thread -skip webengine -nomake tests -nomake examples -no-dbus -no-ssl -no-pch -opensource -confirm-license -prefix "$PWD/../Qt5_binaries"
	make module-qtbase module-qtsvg module-qtcharts -j4
	make install -j4
	cd ..
fi
echo "Qt set up."

## Building the project
echo "Building source repo..."
REPO_FOLDER=$(echo "${SOURCE_REPO}" | grep -oP '[^/]*(?=\.git$)')
if [ -d "${REPO_FOLDER}" ]; then
	echo "Found existing source repo."
else
	echo "Cloning source repo..."
	git clone ${SOURCE_REPO}
fi
cd ${REPO_FOLDER}
../Qt5_binaries/bin/qmake -r MNECPP_CONFIG=wasm
make -j4
GIT_HASH=$(git rev-parse HEAD)
cd ..
echo "Done"

## WASM coi fix
if [ -d "coi-serviceworker" ]; then
	echo "Found existing coi fix repo."
else
	echo "Cloning coi fix repo..."
	git clone https://github.com/gzuidhof/coi-serviceworker.git
fi
cp coi-serviceworker/coi-serviceworker.js mne-cpp/bin
sed -i '0,/.*<script.*/s//\t<script src=\"coi-serviceworker.js\"><\/script>\n&/' mne-cpp/bin/mne_anonymize.html
sed -i '0,/.*<script.*/s//\t<script src=\"coi-serviceworker.js\"><\/script>\n&/' mne-cpp/bin/mne_analyze.html

## Build log
echo "Saving build and system parameters..."
touch build_info.txt
echo "System Info: $(uname -rvop)" >> build_info.txt
echo "emsdk version: ${EMSDK_VERSION}" >> build_info.txt
echo "Qt version: ${QT_VERSION}" >> build_info.txt
echo "Build time: $(date)" >> build_info.txt
echo "Build git hash: ${GIT_HASH}" >> build_info.txt
echo "Saved."

exit 0
