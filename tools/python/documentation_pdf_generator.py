import mne_cpp.core
import mne_cpp.pdf_doc

projectFolder = mne_cpp.core.base_folder()

webBaseFolder = projectFolder + 'doc/gh-pages'

webDocuments = mne_cpp.pdf_doc.scan_folder(webBaseFolder)
# print(webDocuments)

web = mne_cpp.pdf_doc.build_web_structure(webDocuments)
# print('Printing Web Structure:')
# print(web)

docFileName = "mnecpp_doc.tex"
