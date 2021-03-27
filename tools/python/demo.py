import mne_cpp.core
import mne_cpp.pdf_doc


mne_cpp.core.version()

projectFolder = mne_cpp.core.base_folder()

print('The path for this MNE-CPP Project installation is ' + projectFolder)

f = open('c:/projects/mne-cpp/doc/gh-pages/pages/documentation/analyze_dipolefit.md','r',encoding='utf8')

fileText = f.read()

aa = mne_cpp.core.extract_filepaths(fileText,File_ext_separator.)

F = open('file.txt',encoding=)

f.close()

print(aa)

a = 3