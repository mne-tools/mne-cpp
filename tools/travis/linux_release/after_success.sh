#!/bin/bash
#set -ev

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile_release

# zip documentation build artefact 
tar cfvz mne-cpp_doc.tar.gz ./html ./qt-creator_doc

# upload documentation
curl -u $DOC_LOGIN:$DOC_PASSWORD -T mne-cpp_doc.tar.gz ftp://$DOC_REMOTE_SERVER/

# update the docu
#wget -O – -q http://doc.mne-cpp.org/maintenance/update.php