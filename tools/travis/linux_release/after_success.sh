#!/bin/bash
#set -ev

if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then

    # === Copy Libs ===
	QT_LIB_DIR=/opt/qt56/bin
    QT_LIBS=(libQt5Core libQt5Concurrent libQt5Gui libQt5Svg libQt5Widgets libQt5Xml)
    n_elements=${#QT_LIBS[@]}
    for ((i = 0; i < n_elements; i++)); do
        libpath="$QT_LIB_DIR/${QT_LIBS[i]}.*"
        cp $libpath ./lib
    done

    #Master
    if [[ $TRAVIS_BRANCH == 'master' ]]; then
        # === user package ===
        tar cfvz mne-cpp-linux-x86_64-master.tar.gz ./bin ./lib

        # upload artifacts
        curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T mne-cpp-linux-x86_64-master.tar.gz ftp://$REMOTE_SERVER/
    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
        # === user package ===
        tar cfvz mne-cpp-linux-x86_64-1.0.0.tar.gz ./bin ./lib

        # upload artifacts
        curl -u $ONEOO_LOGIN:$ONEOO_PASSWORD -T mne-cpp-linux-x86_64-1.0.0.tar.gz ftp://$REMOTE_SERVER/
    fi
fi