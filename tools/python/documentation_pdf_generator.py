import mne_cpp.core
import mne_cpp.pdf_doc

projectFolder = mne_cpp.core.baseFolder()
webBaseFolder = projectFolder + 'doc/gh-pages'

# webDocuments = mne_cpp.pdf_doc.scanFolder(webBaseFolder)
# print(webDocuments)

# web = mne_cpp.pdf_doc.buildWebStructure(webDocuments)
# print('Printing Web Structure:')
# print(web)

# (pathLabel, filePath, fileName, fileExt, fullPath) = mne_cpp.core.extractFilePaths('../../doc/gh-pages/pages/documentation/anonymize.md')
(pathLabel, filePath, fileName, fileExt, fullPath) = mne_cpp.core.extractFilePaths('../../doc/gh-pages/pages/download/changelog.md')

inFile = open(fullPath, mode = 'r', encoding = 'utf8')
inText = inFile.read()
inFile.close()

outText = mne_cpp.pdf_doc.parseInlineBoldText(inText)
outFile = open(pathLabel + filePath + fileName + '_PROCESSED' + '.' + fileExt, mode = 'w', encoding = 'utf8')
outFile.write(outText)
outFile.close()

