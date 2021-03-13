import sys
from pathlib import Path
from os import scandir, path 
from shutil import copyfile
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPM
from PIL import Image

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
        self.nav_order = 0
        self.has_children = False
        self.nav_exclude = False
        self.fullPath = path
    
    def setTitle(self, title):
        self.title = title

    def setParent(self, parent):
        self.parent = parent

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
        # s += "path: " + self.fullPath + "\n"
        s += "title: " + self.title + "\n"
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
        if d2.parent == self.doc.title:
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
        s = " " * spaces + self.doc.title + "\n"
        for p in self.children:
            s += "  " * spaces + p.print(spaces+2)
        return s

    def __str__(self):
        return self.print()

    def __repr__(self):
        return str(self)      

def parse(s,key):
    if s.lstrip().startswith(key):
        return s.split(":")[1].lstrip()

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

myPath = path.join(currentPath(),"gh-pages")
# print(myPath)
documents = scanFolder(myPath)

print(documents)

web = Page(Document(""))
while documents:
    for d in documents:
        if web.insert(d):
            documents.remove(d)

print("Printing Web Structure:")
print(web)
