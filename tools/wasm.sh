#!/bin/bash

###
# It is recommended you run this script while in an empty directory.
# The script will download and build the necessary components, and 
# reuse them if they are already present. 
###

argc=$# # number of arguments passed (not counting the command itself)
argv=("$@") # list of arguments passed (not including the command itself)

function cleanAbsPath()
{
    local CLEAN_ABS_PATH="$( #spawns a new bash interpreter
        cd "$1" >/dev/null 2>&1 #change directory to that folder
        pwd -P
    )"
    echo "$CLEAN_ABS_PATH"
}

SCRIPT_PATH="$(cleanAbsPath "$(dirname "$0")")"
PROJECT_PATH="$(cleanAbsPath "$SCRIPT_PATH/../..")"

cd ${PROJECT_PATH}

## User editable through flags
SOURCE_REPO=""
EMSDK_VERSION="latest"
QT_VERSION="5.15"

## Stop at first error
set -e

for (( j=0; j<argc; j++)); do
    if [ "${argv[j]}" == "--emsdk" ] || [ "${argv[j]}" == "-e" ]; then
        EMSDK_VERSION="${argv[j+1]}" 
		j=$j+1
    elif [ "${argv[j]}" == "--qt" ] || [ "${argv[j]}" == "-q" ]; then
        QT_VERSION="${argv[j+1]}"
		j=$j+1
    fi
done

## Left for debugging purposes
# echo "${EMSDK_VERSION}"
# echo "${QT_VERSION}"

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

# ../Qt5_binaries/bin/qmake -r MNECPP_CONFIG=wasm
# make -j4
GIT_HASH=$(git rev-parse HEAD)
# cd ..

# echo $QT_DIR
#
# export PATH="/home/gbm/Documents/Code/mne_wasm/Qt5_binaries/bin:$PATH"
# export LD_LIBRARY_PATH="/home/gbm/Documents/Code/mne_wasm/Qt5_binaries/lib:$LD_LIBRARY_PATH"
#

# export CMAKE_PREFIX_PATH="${PROJECT_PATH}/Qt5_binaries:${CMAKE_PREFIX_PATH}"
# export QT_DIR="${PROJECT_PATH}/Qt5_binaries/lib/cmake/Qt5"
# export Qt5_DIR="${PROJECT_PATH}/Qt5_binaries/lib/cmake/Qt5"

# export PATH="${PROJECT_PATH}/Qt5_binaries/bin:$PATH"
# export LD_LIBRARY_PATH="${PROJECT_PATH}/mne_wasm/Qt5_binaries/lib:$LD_LIBRARY_PATH"


# echo "QT DIR = $QT_DIR"
# echo "Qt5_DIR = $Qt5_DIR"
# echo "PREFIX_PATH = ${CMAKE_PREFIX_PATH}"

#emconfigure

QT_CMAKE_FLAGS=""

for d in ${PROJECT_PATH}/Qt5_binaries/lib/cmake/* ; do
	d=$(basename ${d})
    QT_CMAKE_FLAGS="${QT_CMAKE_FLAGS} -D${d}_DIR=${PROJECT_PATH}/Qt5_binaries/lib/cmake/${d}"
done

echo "DIRS ---> ${QT_CMAKE_FLAGS}"

emcmake cmake -B build -S src -DWASM=ON -DQT_DIR=${PROJECT_PATH}/Qt5_binaries/lib/cmake/Qt5 ${QT_CMAKE_FLAGS}

#-DWASM_QT_PATH=${PROJECT_PATH}/Qt5_binaries/lib/cmake/Qt5

cmake --build build

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
