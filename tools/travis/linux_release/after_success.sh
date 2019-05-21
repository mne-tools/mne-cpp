#!/bin/bash
#set -ev

#if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then
    echo -e "Packaging binaries and libs"

    # === Copy Libs ===
	QT_LIB_DIR=/opt/qt510/lib
    QT_LIBS=(libQt5Charts libQt5Concurrent libQt5Core libQt5Gui libQt5Network libQt5PrintSupport libQt5Qml libQt5Svg libQt5Test libQt5Widgets libQt5Xml libQt53DCore libQt53DExtras libQt53DInput libQt53DLogic libQt53DRender )
    n_elements=${#QT_LIBS[@]}
    for ((i = 0; i < n_elements; i++)); do
        libpath="$QT_LIB_DIR/${QT_LIBS[i]}.*"
        cp $libpath ./lib
    done

    # === user package ===
    archive_name="mne-cpp-linux-x86_64-$TRAVIS_BRANCH.tar.gz"
    tar cfvz $archive_name ./bin ./lib

    #Master
    if [[ $TRAVIS_BRANCH == 'master' ]]; then
        # upload artifacts
    	echo -e "Trying old code"
        curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/ --connect-timeout 8 --retry 10 --retry-delay 3
    	echo -e "Trying new code"
        curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/
    	echo -e "Trying new code 1"
        curl -u $MASTER_LOGIN:$MASTER_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/ --retry 10 --retry-delay 3
    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
        # upload artifacts
        curl -u $ONEOO_LOGIN:$ONEOO_PASSWORD -T $archive_name ftp://$REMOTE_SERVER/ --connect-timeout 8 --retry 10 --retry-delay 3
    fi
#fi
