import sys
from pathlib import Path
from os import scandir, path 
from shutil import copyfile
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPM
from PIL import Image

import re

def currentPath():
    return path.abspath(path.dirname(sys.argv[0]))

def extractFilePathNameExt(file):
    basePath = path.split(file)
    filePath = basePath[0]
    fileName = basePath[1].split(".")[0]
    fileExtension = basePath[1].split(".")[1]
    return filePath, fileName, fileExtension

def svg2png(file):
    drawing = svg2rlg(file)
    fPath, fName, _ = extractFilePathNameExt(file)
    pngFile = path.join(fPath, fName + ".png")
    renderPM.drawToFile(drawing, pngFile, fmt="PNG")

def jpg2png(file):
    print(file)
    im1 = Image.open(file)
    fPath, fName, _ = extractFilePathNameExt(file)
    im1.save(path.join(fPath, fName + ".png"))
class Document:
    def __init__(self, path):
        self.title = ""
        self.parent = ""
        self.grand_parent = ""
        self.nav_order = 0
        self.has_children = False
        self.nav_exclude = False
        self.fullPath = path
    
    def setTitle(self, title):
        self.title = title

    def setParent(self, parent):
        self.parent = parent
    
    def setGrandParent(self, grandParent):
        self.grand_parent = grandParent

    def setNavOrder(self, nav_order):
        self.nav_order = nav_order
    
    def setHasChildren(self, hasChildren):
        self.has_children = hasChildren
    
    def setNavExclude(self, navExclude):
        self.nav_exclude = navExclude
    
    def setFullPath(self, path):
        self.fullPath = path

    def __str__(self):
        s = ""
        # s += "--- Class: " + type(self).__name__ + " ---\n"
        s += "title: " + self.title + "\n"
        s += "path: " + self.fullPath + "\n"
        # s += "parent: " + self.parent + "\n"
        # s += "nav_order: " + str(self.nav_order) + "\n"
        # s += "has_children: " + str(self.has_children) + "\n"
        # s += "nav_exclude: " + str(self.nav_exclude) + "\n"
        return s

    def __repr__(self):
        return str(self)
class Page:
    def __init__(self,doc):
        self.doc = doc
        self.children = []
    def insert(self,d2):
        if d2.parent == self.doc.title and \
           d2.grand_parent == self.doc.parent :
            self.children.append(Page(d2))
            self.children.sort(key=lambda p:p.doc.nav_order)
            return True
        else:
            for c in self.children:
                found = c.insert(d2)
                if found:
                    return True
            return False
    def print(self, spaces = 0):
        s = " " * spaces + " - " + self.doc.title + " (" + self.doc.fullPath + ")\n"
        for p in self.children:
            s += "  " * spaces + p.print(spaces+2)
        return s

    def __str__(self):
        return self.print()

    def __repr__(self):
        return str(self)      

def parseFile(file, verboseMode = False):
    with open(file, 'r', encoding="utf8") as fileOpened:
        insideHeader = False
        codeText = False
        validContentFile = False
        doc = Document(file.path)
        for line in fileOpened:
            # print(line)
            if not codeText and (line.startswith("```") or line.count("```")%2 != 0):
                codeText = True
                continue
            if codeText and (line.startswith("```") or line.count("```")%2 != 0):
                codeText = False
                continue
            if line.startswith("---") and not insideHeader:
                insideHeader = True
                continue
            if line.startswith("---") and insideHeader:
                insideHeader = False
                break
            if insideHeader and not codeText:
                if line.lstrip().startswith("title"):
                    doc.setTitle(line.split(":")[1].lstrip().rstrip())
                    validContentFile = True
                    continue
                if line.lstrip().startswith("parent"):
                    doc.setParent(line.split(":")[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith("nav_order"):
                    doc.setNavOrder(int(line.split(":")[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith("has_children"):
                    doc.setHasChildren(bool(line.split(":")[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith("nav_exclude"):
                    doc.setNavExclude(bool(line.split(":")[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith("grand_parent"):
                    doc.setGrandParent(line.split(":")[1].lstrip().rstrip())
                    continue
        if validContentFile and verboseMode:
            print(doc)
    return doc, validContentFile

def scanFolder(folderPath, documents = []):
    for file in scandir(folderPath):
        if file.is_file() and file.name.endswith("md"):
            # print("parsing file: " + file.path)
            doc, valid = parseFile(file)
            if valid:
                documents.append(doc)
        if file.is_dir():
            scanFolder(file, documents)
    return documents

def processImage(imageFile):
    _, _, iExt = extractFilePathNameExt(imageFile)
    if iExt == "jpg" or iExt == "jpeg":
        jpg2png(imageFile)
    if iExt == "svg2":
        svg2png(imageFile)

def recursiveProcess(folderPath, func):
    for file in scandir(folderPath):
        if file.is_dir():
            recursiveProcess(file, func)
        if file.is_file():
            func(file)

# imagesFolder = path.join("gh-pages", "images")
# svg2png(svgFile)
# jpg2png("gh-pages/images/1280px-EEGoSportsGUI.jpg")

webPath = path.join(currentPath(),"../gh-pages")
# print(webPath)
documents = scanFolder(webPath)

print(documents)

web = Page(Document(""))
while documents:
    for d in documents:
        if web.insert(d):
            documents.remove(d)

print("Printing Web Structure:")
print(web)



# instructions on how to install svglib correctly on ubuntu
# sudo apt-get install update
# sudo apt-get install python3
# sudo apt-get install python3-pip
# python3 -m pip install svglib

# then python3 pdfDocumentationGenerator.py

# for w in web:

# shutil.copy(src_dir,dst_dir)
docFileName = "mnecpp_doc.tex"

# outFile = open(path.join(currentPath(),docFileName),"a+")

def parseMarkDownFile(doc, texFile, sectionLevel, verboseMode = False):
    if doc.fullPath == "": 
        return
    else:
        with open(doc.fullPath, 'r', encoding="utf8") as markDownFile, \
             open(texFile,"a+") as texFile:
            insideHeader = False
            headerBehind = False
            insideHtml = False
            insideUnorderedList = False
            insideOrderedList = False
            insideTable = False
            if verboseMode:
                print("Parsing file: " + doc.fullPath)
            for line in markDownFile:
                if not headerBehind:
                    if not insideHeader and line.startswith("---"):
                        insideHeader = True
                    continue
                    if line.startswith("---"):
                        insideHeader = False
                        headerBehind = True
                    continue
                else:
                    if line.count("<html>") > line.count("</html>"):
                        insideHtml = True
                    if line.count("</html>") >= line.count("<html>"):
                        insideHtml = False
                    if not insideHeader and not insideHtml:
                        if   parseHeader(texFile,line,"# ","part","sec"):
                            continue
                        elif parseHeader(texFile,line,"## ","section","sec"):
                            continue
                        elif parseHeader(texFile,line,"### ","subsection","ssec"):
                            continue
                        elif parseHeader(texFile,line,"#### ","subsubsection","ssec"):
                            continue
                        elif parseImageFigure(texFile,line):
                            continue
                        else:
                            lineOut = parseBoldMd(line)
                            lineOut = parseItalicMd(lineOut)

def parseHeader(texFile,str,markdownKey,latexKey,labelLatexKey):
    if str.startswith(markdownKey):
        newHeader = line.split(markdownKey)[1].ltrip().rstrip()
        texFile.write("\n\\" + latexKey + "{" + newHeader.strip() + "}" + " \n\\label{" + labelLatexKey + ":" + newHeader.strip().replace(" ","_") + "}")
        return True
    else:
        return False

def parseImageFigure(texFile,str):
    if str.startswith("!["):
        captionText = str.split("[")[1].split("]")[0]
        imageFile = str.split("(")[1].split(")")[0]
        texFile.write("\n\\begin{figure}[h]")
        texFile.write("\n\\centering \\includegraphics[width=0.5\\linewidth]{" + imageFile + "}")
        texFile.write("\n\\caption{" + captionText + ".}")      
        texFile.write("\n\\label:{fig:" + captionText.replace(" ","_") + "}")      
        texFile.write("\n\\end{figure}")
        return True
    else:
        return False

def parseBoldMd(str):
    if str.count("**") == 2:
        strSplitted = str.split("**")
        strOut = strSplitted[0] + "\\textbf{" + strSplitted[1].split("**")[0] + "}" + strSplitted[1].split("**")[1]
    elif str.count("__") == 2:
        strSplitted = str.split("__")
        strOut = strSplitted[0] + "\\textbf{" + strSplitted[1].split("__")[0] + "}" + strSplitted[1].split("__")[1]        
    else:
        strOut = str
    return strOut

def parseItalicMd(str):
    if str.count("*") == 2:
        strSplitted = str.split("*")
        strOut = strSplitted[0] + "\\textit{" + strSplitted[1].split("*")[0] + "}" + strSplitted[1].split("*")[1]
    elif str.count("_") == 2:
        strSplitted = str.split("_")
        strOut = strSplitted[0] + "\\textit{" + strSplitted[1].split("_")[0] + "}" + strSplitted[1].split("_")[1]
    else:
        strOut = str
    return strOut

# def parseEmbededPdf(str):

# def parseTableMd(str)

def parseWeb(web, sectionLevel = 0):
    print("Parsing file: " + web.doc.fullPath)
    parseMarkDownFile(web.doc, "teseta.tex", sectionLevel)
    for p in web.children:
        parseWeb(p,sectionLevel+1)

def parseLinks(str):
    str = str.replace("{:target=\"_blank\" rel=\"noopener\"}","")
    nameRe = "[^]]+"
    urlRe = "http[s]?://[^)]+"
    markupRegex = '\[({0})]\(\s*({1})\s*\)'.format(nameRe, urlRe)
    linkList = re.findall(markupRegex, str)
    if linkList:
        link = linkList[0]
        strPre  = str.split("[" + link[0] + "]")[0]
        strPost = str.split("(" + link[1] + ")")[1]
        strLink = "\\href{" + link[1] + "}{" + link[0] + "}\\footnote{" + link[1] + "}"
        strOut = strPre + strLink + strPost
        parseLinks(strOut)
    else:
        return str

def parseImagesInline(str):
    textRe = "[^]]*"
    imgRe = "\/.*?\.[\w:]+"
    markupRegex = '!\[({0})]\(\s*({1})\s*\)'.format(textRe, imgRe)
    imgList = re.findall(markupRegex, str)
    if imgList:
        img = imgList[0]
        strPre  = str.split("![" + img[0] + "]")[0]
        strPost = str.split("(" + img[1] + ")")[1]
        strImg = "\\begin{minipage}{.3\\textwidth}\n\\includegraphics[width=\linewidth,height=60mm]{" + img[1] + "}\n\\end{minipage}"
        strOut = strPre + strImg + strPost
        parseImagesInline(strOut)
    else:
        return str

# parseWeb(web)



# !\[[^\]]*\]
# '(startText)(.+)((?:\n.+)+)(endText)'
# \href{https://mne-cpp.github.io}{MNE-CPP Project documentation web page}\footnote{https://mne-cpp.github.io}

# filePath = path.join(currentPath(),"../gh-pages/pages/development/wasm_buildguide.md")

# with open(filePath, 'r', encoding="utf8") as mdFile:
#     for line in mdFile:
#         newLine = parseLinks(line)
#         print(newLine)


    # \begin{minipage}{.3\textwidth}
    #   \includegraphics[width=\linewidth, height=60mm]{tiger}
    # \end{minipage}


# still missing: 
# table parsing
# unordered lists parsing
# ordered lists parsing
# inbound links vs outbound links
# parse html image inline tags
