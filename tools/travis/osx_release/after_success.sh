#!/bin/bash

if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then

	#Extract Qt dirs from actual version
    QMAKE_VERSION=`qmake -v`
    QT_LIB_DIR=${QMAKE_VERSION##*in }
    QT_BIN_DIR=${QT_LIB_DIR%/*}/bin
 
    MNE_LIB_DIR=$(pwd)/lib

    PATH=$QT_BIN_DIR:$PATH
    export PATH

    DYLD_LIBRARY_PATH=$MNE_LIB_DIR:$DYLD_LIBRARY_PATH
    export DYLD_LIBRARY_PATH
    DYLD_FALLBACK_LIBRARY_PATH=$MNE_LIB_DIR
    export DYLD_FALLBACK_LIBRARY_PATH


    # === Deployment ===
    TANGIBLES=(mne_x mne_browse_raw_qt mne_analyze_qt)
    n_elements=${#TANGIBLES[@]}
    for ((i = 0; i < n_elements; i++)); do
        fixfile="./bin/${TANGIBLES[i]}.app/Contents/MacOS/${TANGIBLES[i]}"
        destdir="./bin/${TANGIBLES[i]}.app/Contents/Frameworks/"

        dylibbundler -od -b -x $fixfile -d $destdir -p @executable_path/../Frameworks/
		
        tangible="./bin/${TANGIBLES[i]}.app"
        macdeployqt $tangible -dmg
	done

	
    # === Copy Tangibles ===
    if [[ $TRAVIS_BRANCH == 'master' ]]; then
        for ((i = 0; i < n_elements; i++)); do
            cp "./bin/${TANGIBLES[i]}.dmg" "./"
            tangible_name="${TANGIBLES[i]}-$TRAVIS_BRANCH.dmg"
            mv "./${TANGIBLES[i]}.dmg" $tangible_name
            # upload artifacts
            curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $tangible_name ftp://$REMOTE_SERVER/
        done
    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
        for ((i = 0; i < n_elements; i++)); do
            cp "./bin/${TANGIBLES[i]}.dmg" "./"
            tangible_name="${TANGIBLES[i]}-$TRAVIS_BRANCH.dmg"
            mv "./${TANGIBLES[i]}.dmg" $tangible_name
            # upload artifacts
            curl -u $ONEOO_LOGIN:$ONEOO_PASSWORD -T $tangible_name ftp://$REMOTE_SERVER/
        done
    fi
fi