# print("Hello world!")
from pathlib import Path
from os import scandir, path
from shutil import copyfile
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPM
from PIL import Image

def extractPathNameExt(file):
    basePath = path.split(file)
    filePath = basePath[0]
    fileName = basePath[1].split(".")[0]
    fileExtension = basePath[1].split(".")[1]
    return filePath, fileName, fileExtension

def svg2png(file):
    drawing = svg2rlg(file)
    fPath, fName, _ = extractPathNameExt(file)
    pngFile = path.join(fPath, fName + ".png")
    # renderPM.drawToFile(drawing, pngFile, fmt="PNG")

def jpg2png(file):
    print(file)
    im1 = Image.open(file)
    fPath, fName, _ = extractPathNameExt(file)
    im1.save(path.join(fPath, fName + ".png"))

class Page:

    def __init__(self, title):
        self.title = title
        self.parentTitle = ""
        self.nav_order = 0
        self.has_children = False
        self.nav_exclude = False
        self.fullPath = ""

def parse(s,key):
    if s.lstrip().startswith(key):
        return s.split(":")[1].lstrip()

def parseFile(file):
    with open(file, 'r', encoding="utf8") as fileOpened:
        isInsideHeader = False
        isCodeText = False
        for line in fileOpened:
            if not isCodeText and line.startswith("```") or line.count("```")%2 != 0:
                isCodeText = True
                continue
            if isCodeText and line.startswith("```") or line.count("```")%2 != 0:
                isCodeText = False
                continue
            if line.startswith("---") and not isInsideHeader:
                isInsideHeader = True
                continue
            if line.startswith("---") and isInsideHeader:
                isInsideHeader = False
                break
            if isInsideHeader:
                if line.lstrip().startswith("title"):
                    print("title: " + line.split(":")[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith("parent"):
                    print("parent: " + line.split(":")[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith("nav_order"):
                    print("nav_order: " + line.split(":")[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith("has_children"):
                    print("has_children: " + line.split(":")[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith("nav"):
                    print("nav: " + line.split(":")[1].lstrip().rstrip())
                    continue

def scanFolder(folderPath):
    for file in scandir(folderPath):
        if file.is_file() and file.name.endswith("md") and file.name != "README.md":
            print(">>>>")
            print(file.path)
            parseFile(file)
        if file.is_dir():
            scanFolder(file)

# myPath = path.join("doc","gh-pages")
# scanFolder(myPath)

def processImage(imageFile):
    _, _, iExt = extractPathNameExt(imageFile)
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




