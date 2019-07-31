#!/bin/bash
#set -ev

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile

#if [[ "${TRAVIS_PULL_REQUEST}" == "false" ]]; then
    git clone -b gh-pages --single-branch --no-checkout --depth 1 https://github.com/mne-tools/mne-cpp mne_cpp.gh-pages
	touch build/mne_cpp.github.io/.nojekyll

	cp -r html/* mne_cpp.gh-pages
	cd mne_cpp.gh-pages
	git add *
	git add .nojekyll
	git commit -a -m 'Build docu'
	git push https://${GIT_TOKEN}@github.com/mne-tools/mne-cpp.git --all
#fi