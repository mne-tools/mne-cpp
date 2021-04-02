import mne_cpp.core
# import mne_cpp.pdf_doc
from os import stat

mne_cpp.core.version()

projectFolder = mne_cpp.core.baseFolder()

#  Recursively list all the files in a directory and order by size and print results.
listOfFiles = []
mne_cpp.core.recursiveFolderProcess(projectFolder + 'doc/gh-pages', lambda f:                     \
                                                                        listOfFiles.append((f, stat(f).st_size))    \
                                                                        if f.name.endswith('.md')  \
                                                                        else None )
listOfFiles.sort(reverse=True, key=lambda f:f[1])
for f in listOfFiles:
    print('File: ' + f[0].path + ' - (' + mne_cpp.core.sizeHumanReadable(f[1]) + ')')

