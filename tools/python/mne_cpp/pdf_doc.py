#=============================================================================================================
#
# @file     pdf_doc.py
# @author   jgarciaprieto@mgh.harvard.edu
# @since    0.1.9
# @date     March, 2021
#
# @section  LICENSE
#
# Copyright (C) 2017, Juan Garcia-Prieto. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    MNE-CPP Project pdf documentation generation from the website.
#
import sys
from pathlib import Path
from os import scandir, path 
import re
import mne_cpp.core
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPM
from PIL import Image

def parseFile(file, verboseMode = False):
    with open(file, 'r', encoding='utf8') as fileOpened:
        insideHeader = False
        codeText = False
        validContentFile = False
        doc = Document(file.path)
        for line in fileOpened:
            if not codeText and (line.startswith('```') or line.count('```')%2 != 0):
                codeText = True
                continue
            if codeText and (line.startswith('```') or line.count('```')%2 != 0):
                codeText = False
                continue
            if line.startswith('---') and not insideHeader:
                insideHeader = True
                continue
            if line.startswith('---') and insideHeader:
                insideHeader = False
                break
            if insideHeader and not codeText:
                if line.lstrip().startswith('title'):
                    doc.setTitle(line.split(':')[1].lstrip().rstrip())
                    validContentFile = True
                    continue
                if line.lstrip().startswith('parent'):
                    doc.setParent(line.split(':')[1].lstrip().rstrip())
                    continue
                if line.lstrip().startswith('nav_order'):
                    doc.setNavOrder(int(line.split(':')[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith('has_children'):
                    doc.setHasChildren(bool(line.split(':')[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith('nav_exclude'):
                    doc.setNavExclude(bool(line.split(':')[1].lstrip().rstrip()))
                    continue
                if line.lstrip().startswith('grand_parent'):
                    doc.setGrandParent(line.split(':')[1].lstrip().rstrip())
                    continue
        if validContentFile and verboseMode:
            print(doc)
    return doc, validContentFile

def scanFolder(folderPath, documents = []):
    for file in scandir(folderPath):
        if file.is_file() and file.name.endswith('md'):
            # print('parsing file: ' + file.path)
            doc, valid = parseFile(file)
            if valid:
                documents.append(doc)
        if file.is_dir():
            scanFolder(file, documents)
    return documents

class Document:
    def __init__(self, path):
        self.title = ''
        self.parent = ''
        self.grand_parent = ''
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
        s = ''
        # s += '--- Class: ' + type(self).__name__ + ' ---\n'
        s += 'title: ' + self.title + ' - '
        s += 'path: ' + self.fullPath + '\n'
        # s += 'parent: ' + self.parent + '\n'
        # s += 'nav_order: ' + str(self.nav_order) + '\n'
        # s += 'has_children: ' + str(self.has_children) + '\n'
        # s += 'nav_exclude: ' + str(self.nav_exclude) + '\n'
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
        s = ' ' * spaces + ' - ' + self.doc.title + ' (' + self.doc.fullPath + ')\n'
        for p in self.children:
            s += ' ' * spaces + p.print(spaces+2)
        return s

    def __str__(self):
        return self.print()

    def __repr__(self):
        return str(self)      

def buildWebStructure(documents):
    web = Page(Document(''))
    while documents:
        for d in documents:
            if web.insert(d):
                documents.remove(d)
    return web

def parseWeb(web, sectionLevel = 0):
    print('Parsing file: ' + web.doc.fullPath)
    parseMarkDownFile(web.doc, latexFile = 'teseta.tex', sectionLevel = 0, verboseMode = True)
    for p in web.children:
        parseWeb(p,sectionLevel = sectionLevel+1, verboseMode = True)

def parseMarkDownFile(file, **inputArgs):
    opts = (('latexFile', 'mne_pdf_manual.tex'),
            ('verboseMode', False), 
            ('sectionLevel', 0))
    (texFile, verboseMode, sectionLevel) = mne_cpp.core.parseInputArgs(inputArgs, opts = opts)
    if file.fullPath == '': 
        return
    else:
        with open(file.fullPath, 'r', encoding='utf8') as markDownFile, \
             open(texFile,'a+') as texFile:
            inText = markDownFile.read()
            inText = deleteJustTheDocsHeader(inText)
            inText = parseInlineItalicText(inText)
            inText = parseInlineBoldText(inText)
            inText = parseUnorderedList(inText)
            inText = parseInlineImages(inText)
            inText = parseInlineHTMLImages(inText)
            inText = parseTableMd(inText)
            inText = parseLinks(inText)
            # inText = parseFigureImages(inText)
            # inText = parseHeaders(inText)

def deleteJustTheDocsHeader(text):
    return re.sub(r'\n*\s*---\s*\n(.*\n)*---\n','',text)

def parseInlineItalicText(text):
    return re.sub(r'(?<=\W)((?P<star>\*)|_)(?P<itext>\w+)(?(star)\*|_)(?=\W)',r'\\textit{\g<itext>}', text)

def parseInlineBoldText(text):
    return re.sub(r'(?<=\W)((?P<dstar>\*\*)|__)(?P<btext>[\w ]+)((?(dstar)\*\*)|__)(?=\W)',r'\\textbf{\g<btext>}', text)

def parseUnorderedList(inText):
    match = re.search(r'(\n\s?\*\s?.+)(\n\s?\*\s?(.+))*', inText)
    if match:
        outList = '\n\\begin{itemize}\n'
        pattern2 = re.compile(r'\n*\s*\*\s*(?P<item>.+)(?=\n)?')
        itemList = pattern2.finditer(match.group(0))
        for item in itemList:
            outList += '\t\\item ' + item.group('item') + '\n'
        outList += '\\end{itemize}'
        outText = inText[:match.start(0)] + outList + inText[match.end(0):]
        return parseUnorderedList(outText)
    else:
        return inText

def parseInlineImages(inText):
    match = re.search(r'!\[(?P<alt_text>[^]]+)\]\((?P<imgFilePath>[^)]+)\)', inText)
    if match:
        imgPath = mne_cpp.core.__ifNoneEmptyStr(match.group('imgFilePath'))
        imgAltText = mne_cpp.core.__ifNoneEmptyStr(match.group('alt_text'))
        figText  = '\n\\begin{wrapfigure}{r}{0.5\\textwidth}'
        figText += '\n\t\\begin{center}'
        figText += '\n\t\t\\includegraphics[width=0.4\\textwidth]{ ' + imgPath + '}'
        figText += '\n\t\\end{center}'
        figText += '\n\t\\caption{' + imgAltText.replace(' ','_') + '}'
        figText += '\n\\end{wrapfigure}'
        outText = inText[:match.start(0)] + figText + inText[match.end(0):]
        return parseInlineImages(outText)
    else:
        return inText

def parseInlineHTMLImages(inText):
    match = re.search(r'<\s*img\s*src\s*=\s*"(?P<imgPath>[^"]+)".*>', inText)
    if match:
        imgPath = mne_cpp.core.__ifNoneEmptyStr(match.group('imgFilePath'))
        figText  = '\n\\begin{wrapfigure}{r}{0.5\\textwidth}'
        figText += '\n\t\\begin{center}'
        figText += '\n\t\t\\includegraphics[width=0.4\\textwidth]{ ' + imgPath + '}'
        figText += '\n\t\\end{center}'
        figText += '\n\t\\caption{' + imgAltText.replace(' ','_') + '}'
        figText += '\n\\end{wrapfigure}'
        outText = inText[:match.start(0)] + figText + inText[match.end(0):]
        return parseInlineImages(outText)
    else:
        return inText

def parseTableMd(inText)
    match = re.search(r'(?<=\n)\|([^|\n]+\|)+', inText)
    if match:
        tableText = inText[match.start(0):match.end(0)]
        firstRow = re.sub(r'^.*',r'\g<0>', tableText) 
        numCols = firstRow.count('|') - 1
        latexTableText  = '\\begin{table}[h!]\n'
        latexTableText += '\\centering\n'
        latexTableText += '\\begin{tabular}{||' + 'c' * numCols + '||}\n'
        latexTableText += '&'.join(firstRow.split('|')[1:-1]) + '\\\\[0.5ex]\n'
        otherRows = re.findall(r'(?<=\n)\|.*', tableText)
        for row in otherRows:
            hColumnMatch = re.search('(?<=\n)\|[:\-|]+',row)
            if hColumnMatch:
                latexTableText += ' \\hline\n'
            latexTableText += ' ' + '&'.join(row.split('|')[1:-1]) + '\\\\\n'
        latexTableText += ' \\hline\n'
        latexTableText += '\\end{tabular}\n'
        latexTableText += '\\end{table}\n'
        outText = inText[:match.start(0)] + latexTableText + inText[match.end(0):]
        return parseInlineImages(outText)
    else:
        return inText

def parseLinks(inText):
    inText = inText.replace('{:target=\"_blank\" rel=\"noopener\"}','')
    nameRe = '[^]]+'
    urlRe = 'http[s]?://[^)]+'
    markupRegex = '\[({0})]\(\s*({1})\s*\)'.format(nameRe, urlRe)
    linkList = re.findall(markupRegex, inText)
    if linkList:
        link = linkList[0]
        inTextPre  = inText.split('[' + link[0] + ']')[0]
        inTextPost = inText.split('(' + link[1] + ')')[1]
        latexLink = '\\href{' + link[1] + '}{' + link[0] + '}\\footnote{' + link[1] + '}'
        strOut = inTextPre + latexLink + inTextPost
        parseLinks(strOut)
    else:
        return str

# still missing: 
# ordered lists parsing
# inbound links vs outbound links
# parse inline code
# parse headers
# preamble and ending file

def parseHeader(texFile,str,markdownKey,latexKey,labelLatexKey):
    if str.startswith(markdownKey):
        newHeader = line.split(markdownKey)[1].ltrip().rstrip()
        texFile.write("\n\\" + latexKey + "{" + newHeader.strip() + "}" + " \n\\label{" + labelLatexKey + ":" + newHeader.strip().replace(" ","_") + "}")
        return True
    else:
        return False

# if   parseHeader(texFile,line,"# ","part","sec"):
#     continue
# elif parseHeader(texFile,line,"## ","section","sec"):
#     continue
# elif parseHeader(texFile,line,"### ","subsection","ssec"):
#     continue
# elif parseHeader(texFile,line,"#### ","subsubsection","ssec"):
#     continue

def processImage(imageFile):
        _, _, _, _, fileExt = mne_cpp.core.parseFilePathNameExt(imageFile)
    if fileExt == "jpg" or fileExt == "jpeg":
        jpg2png(imageFile)
    if fileExt == "svg2":
        svg2png(imageFile)

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


# imagesFolder = path.join("gh-pages", "images")
# svg2png(svgFile)
# jpg2png("gh-pages/images/1280px-EEGoSportsGUI.jpg")

# instructions on how to install svglib correctly on ubuntu
# sudo apt-get install update
# sudo apt-get install python3
# sudo apt-get install python3-pip
# python3 -m pip install svglib

# then python3 pdfDocumentationGenerator.py

# for w in web:

# shutil.copy(src_dir,dst_dir)

# outFile = open(path.join(currentPath(),docFileName),"a+")

