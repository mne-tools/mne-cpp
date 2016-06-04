#!/bin/bash
#set -ev

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile_release

# zip documentation build artefact 
tar cfvz mne-cpp_doc.tar.gz ./html ./qt-creator_doc

# upload documentation
curl -T mne-cpp_doc.tar.gz -u $DOC_LOGIN:$DOC_PASSWORD $DOC_REMOTE_SERVER
