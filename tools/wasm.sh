#!/bin/bash

# aqt install-qt linux desktop 6.5.0 wasm_multithread -m qtcharts --autodesktop

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

EMSDK_VERSION="latest"
SAVE_LOG="true"
SHOW_HELP="false"

SCRIPT_PATH="$(cleanAbsPath "$(dirname "$0")")"
PROJECT_BASE_PATH="$(cleanAbsPath "$SCRIPT_PATH/..")"

NUM_PARALLEL_PROC=1

if [ "$(uname)" == "Darwin" ]; then
    NUM_PARALLEL_PROC=$(sysctl -n hw.logicalcpu)
else
    NUM_PARALLEL_PROC=$(expr $(nproc --all))
fi

## Stop at first error
set -e

for (( j=0; j<argc; j++)); do
    if [ "${argv[j]}" == "--emsdk" ] || [ "${argv[j]}" == "-e" ]; then
        EMSDK_VERSION="${argv[j+1]}" 
		j=$j+1
	elif [ "${argv[j]}" == "--no-log" ]; then
		SAVE_LOG="false"
	elif [ "${argv[j]}" == "--help" ] || [ "${argv[j]}" == "-h" ]; then
		SHOW_HELP="true"
	fi
done

if [ ${SHOW_HELP} == "true" ]; then
	echo "MNE-CPP wasm build script"
	echo ""
	echo "Usage:"
	echo "    wasm --emsdk <VERSION>"
	echo ""
	echo "Depends on the enviroment variable Qt6_DIR being set and having wasm qt installed."
	echo "For more info, go to https://doc.qt.io/qt-6/wasm.html"
	echo ""
	echo "Options:"
	echo "    --emsdk, -e  Set what version of emscripten to use."
	echo "                 Qt recommends the following versions depending"
	echo "                 on what version of Qt you use:"
	echo "                    Qt 6.2 -> emsdk 2.0.14"
	echo "                    Qt 6.3 -> emsdk 3.0.0"
	echo "                    Qt 6.4 -> emsdk 3.1.14"
	echo "                    Qt 6.5 -> emsdk 3.1.25"
	echo ""
  	echo "    --no-log     Omits build log."
	echo ""
	echo "    --help, -h   Prints this help text."

	exit 0
fi

# Make sure we run everything relative to root folder as pwd
cd ${PROJECT_BASE_PATH}

## Emscripten
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

## Build project
SOURCE_DIRECTORY=${PROJECT_BASE_PATH}/src
BUILD_DIRECTORY=${PROJECT_BASE_PATH}/build/wasm
OUTPUT_DIRECTORY=${PROJECT_BASE_PATH}/out/wasm

${Qt6_DIR}/wasm_multithread/bin/qt-cmake \
	-DQT_HOST_PATH=${Qt6_DIR}/gcc_64 \
	-DCMAKE_BUILD_TYPE=Release \
	-DBINARY_OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY} \
	-S ${SOURCE_DIRECTORY} \
	-B ${BUILD_DIRECTORY} \
	-DWASM=ON

cmake \
	--build ${BUILD_DIRECTORY} \
	--parallel ${NUM_PARALLEL_PROC}

## Log
if [ ${SAVE_LOG} == "true" ]; then
	echo "Saving build and system parameters..."
	touch build_info.txt
	echo "System Info: $(uname -rvop)" >> build_info.txt
	echo "emsdk version: ${EMSDK_VERSION}" >> build_info.txt
	echo "Qt version: ${QT_VERSION}" >> build_info.txt
	echo "Build time: $(date)" >> build_info.txt
	echo "Build git hash: ${GIT_HASH}" >> build_info.txt
	echo "Saved."
fi

exit 0
