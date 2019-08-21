#!/bin/bash
#set -ev

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile

if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then
	# Upload docu to gh-pages
    	git clone -b gh-pages --single-branch --no-checkout --depth 1 https://github.com/mne-tools/mne-cpp gh-pages
	cd gh-pages

	# Remove all files first
	git rm * -r
	git commit --amend -a -m 'Update docu'

	touch .nojekyll
	
	cp -r ../html/* .

	git add *
	git add .nojekyll
	git commit --amend -a -m 'Update docu'
	git push -f https://${GIT_TOKEN}@github.com/mne-tools/mne-cpp.git --all

	# Upload Qt Creator qch file to Azure
	cd ../..

        ./azcopy copy doc/qt-creator_doc/mne-cpp.qch "$REMOTE_AZURE_SERVERmne-cpp.qch?sv=2018-03-28&ss=b&srt=o&sp=w&se=2022-05-31T02:17:52Z&st=2019-05-30T18:17:52Z&spr=https&sig=$SAS"
fi
