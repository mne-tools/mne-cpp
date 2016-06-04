#!/bin/bash
#set -ev

# add path to qhelpgenerator
PATH=$PATH:/usr/lib/x86_64-linux-gnu/qt5/bin/

# Run Doxygen
cd doc
doxygen mne-cpp_doxyfile_release

# zip documentation build artefact 
tar cfvz mne-cpp_doc.tar.gz ./html ./qt-creator_doc

# upload documentation
ftp -n $DOC_REMOTE_SERVER <<INPUT_END
quote user $DOC_LOGIN
quote pass $DOC_PASSWORD
prompt off
mput mne-cpp_doc.tar.gz
exit
INPUT_END
