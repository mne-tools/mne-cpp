import mne_cpp.core as mne
# import mne_cpp.pdf_doc
from os import stat

mne.version()

projectFolder = mne.baseFolder()

#  Recursively list all the files in a directory and order by size and print results.
listOfFiles = []
mne.recursiveFolderProcess(projectFolder + 'doc/gh-pages', lambda f:                     \
                                                            listOfFiles.append((f, stat(f).st_size))    \
                                                            if f.name.endswith('.md')  \
                                                            else None )
listOfFiles.sort(reverse=True, key=lambda f:f[1])
for f in listOfFiles:
    print('File: ' + f[0].path + ' - (' + mne.sizeHumanReadable(f[1]) + ')')

