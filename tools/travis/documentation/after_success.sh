#!/bin/bash
#set -ev

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile

# zip documentation build artefact 
#tar cfvz mne-cpp_doc.tar.gz ./html ./qt-creator_doc
zip -r mne-cpp_doc.zip ./html ./qt-creator_doc

# upload documentation
#curl -u $DOC_LOGIN:$DOC_PASSWORD -T mne-cpp_doc.tar.gz ftp://$REMOTE_SERVER/
curl -u $DOC_LOGIN:$DOC_PASSWORD -T mne-cpp_doc.zip ftp://$REMOTE_SERVER/

# update the docu
wget -O – -q http://doc.mne-cpp.org/maintenance/update.php