#!/bin/bash
#set -ev
if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then
    echo -e "Packaging binaries and libs"

    # === Linux Deploy Qt ===
	#sourcing qt510 again, this is for linuxdeployqt; verified redundant, so commented out. 
	#source /opt/qt510/bin/qt510-env.sh

	#setting linuxdeployqt to variable
	linuxdeployqt=linuxdeployqt-continuous-x86_64.AppImage

	#archive file name created
	archive_name="mne-cpp-linux-x86_64-$TRAVIS_BRANCH.tar.gz"

	#copying built data to folder for easy packaging 	
	cp -r ./bin ./lib mne-cpp/

	#dropping into folder to easily package all results from linuxdeployqt
	cd mne-cpp

	#linuxdeployqt uses mne_scan binary to resolve dependencies in current directory. 
	../$linuxdeployqt bin/mne_scan

	#creating archive of everything in current directory
	tar cfvz ../$archive_name ./*
	
	#Moving up one directory level.
	cd .. 

    #Master
    if [[ $TRAVIS_BRANCH == 'master' ]]; then
        # upload artifacts
        ./azcopy copy $archive_name "$REMOTE_AZURE_SERVER$archive_name?sv=2018-03-28&ss=b&srt=o&sp=w&se=2022-05-31T02:17:52Z&st=2019-05-30T18:17:52Z&spr=https&sig=$SAS"
    elif [[ $TRAVIS_BRANCH == '1.0.0' ]]; then
        # upload artifacts
        ./azcopy copy $archive_name "$REMOTE_AZURE_SERVER$archive_name?sv=2018-03-28&ss=b&srt=o&sp=w&se=2022-05-31T02:17:52Z&st=2019-05-30T18:17:52Z&spr=https&sig=$SAS"
    fi
fi
