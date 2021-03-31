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

def parseFile(file, verboseMode = False):
    with open(file, 'r', encoding='utf8') as fileOpened:
        insideHeader = False
        codeText = False
        validContentFile = False
        doc = Document(file.path)
        for line in fileOpened:
            # print(line)
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
    print("Parsing file: " + web.doc.fullPath)
    parseMarkDownFile(web.doc, latexFile = "teseta.tex", sectionLevel = 0, verboseMode = True)
    for p in web.children:
        parseWeb(p,sectionLevel = sectionLevel+1, verboseMode = True)

def parseMarkDownFile(file, **inputArgs):
    opts = (('latexFile', 'mne_pdf_manual.tex'),
            ('verboseMode', False), 
            ('sectionLevel', 0))
    (texFile, verboseMode, sectionLevel) = mne_cpp.core.parseInputArgs(inputArgs, opts = opts)
    if file.fullPath == "": 
        return
    else:
        with open(file.fullPath, 'r', encoding="utf8") as markDownFile, \
             open(texFile,"a+") as texFile:
            inText = markDownFile.read()
            inText = deleteJustTheDocsHeader(inText)



def deleteJustTheDocsHeader(text):
    return re.sub(r'\n*\s*---\s*\n(.*\n)*---\n','',text)

def parseInlineItalicText(text):
    return re.sub(r'(?<=\W)((?P<star>\*)|_)(?P<itext>\w+)(?(star)\*|_)(?=\W)',r'\\textit{\g<itext>}', text)

def parseInlineBoldText(text):
    return re.sub(r'(?<=\W)((?P<dstar>\*\*)|__)(?P<btext>\w+)((?(dstar)\*\*)|__)(?=\W)',r'\\textbf{\g<btext>}', text)    