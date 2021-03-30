#=============================================================================================================
#
# @file     mne_cpp.py
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
# @brief    MNE-CPP Project Python main module.
#
import sys
from os import path, scandir
from enum import Enum
import re

# This file should be kept inside the following folder: mne_mneBaseFolder/tools/python/mne_cpp

def version():
    print('MNE-CPP Project python module. - Version: 0.1.9')
    print('Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.')

def __currentPath():
    return path.abspath(path.dirname(sys.argv[0]))

def baseFolder():
    this_script_path = __currentPath().split(path.sep)
    project_path = ''
    for f in this_script_path[:-2]:
        project_path += f + '/'
    return project_path
class File_ext_separator(Enum):
    FIRST = 1
    LAST = 2

def extractFilePaths(text: str, **inputArgs):
    
    (fileExtensionSeparator, ) = parseInputArguments(inputArgs, opts = (('fileExtensionSeparator', 'last'), ))

    if fileExtensionSeparator == 'first':    # first '.' in filename separates ext
        expression = re.compile(r"""
                                (?P<deviceLabel>[A-Za-z]:)?     # Device Label. c:  D: etc... for windows.
                                (?P<filePath>([/\\]?)           # filePath: absolute or relative path. /folder/file.txt vs folder/file.txt
                                                                #                                      ^                   ^         
                                 (?P<lastFolder>(\.{1,2}|[^\n.<>:"|?*/\\]+)[/\\])+  # An undefined number of nested folders '.' and '..' admitted. 
                                                                                    # The forbidden char set is mostly due to windows = [^\n.<>:"|?*/\\] but 
                                                                                    # using them in linux is kind of asking for trouble. So out!
                                )                                                                                    
                                (?P<fileName>([^/\\{}\n?*":\' ][^/\\{}\n?*":\'][^/\\{}\n?*":\' .]*?)+?)  #notice the not greedy. Thats what makes it first select the first '.'
                                \.       #
                                (?P<fileExtension>[A-Za-z0-9_.]*)                  
                                """,re.X)
    elif fileExtensionSeparator == 'last':   # last '.' in the filename separates the file extension
        expression = re.compile(r"""
                                (?P<deviceLabel>[A-Za-z]:)?     # Device Label. c:  D: etc... for windows.
                                (?P<filePath>([/\\]?)           # filePath: absolute or relative path. /folder/file.txt vs folder/file.txt
                                                                #                                      ^                   ^         
                                 (?P<lastFolder>(\.{1,2}|[^\n.<>:"|?*/\\]+)[/\\])+  # An undefined number of nested folders '.' and '..' admitted. 
                                                                                    # The forbidden char set is mostly due to windows = [^\n.<>:"|?*/\\] but 
                                                                                    # using them in linux is kind of asking for trouble. So out!
                                )                                                                                    
                                (?P<fileName>([^/\\{}\n?*":\' ][^/\\{}\n?*":\'][^/\\{}\n?*":\' .]*?)+)
                                \.       #this will be surely the last '.' in a possibly "multy" extension file 
                                (?P<fileExtension>[A-Za-z0-9_.]*)                  
                                """,re.X)
    else: 
        raise NameError('Unkown extension separator option used: ' + str(fileExtensionSeparator))
    pattern = re.compile(expression)
    fileMatches = pattern.finditer(text)
    for f in fileMatches:
        deviceLabel = __ifNoneEmptyStr(f.group('deviceLabel'))    #f.string[f.start('deviceLabel'):f.end('deviceLabel')]
        filePath = __ifNoneEmptyStr(f.group('filePath'))          #f.string[f.start('filePath'):f.end('filePath')-1]
        fileName = __ifNoneEmptyStr(f.group('fileName'))          #f.string[f.start('fileName'):f.end('fileName')]
        fileExt = __ifNoneEmptyStr(f.group('fileExtension'))      #f.string[f.start('fileExtension'):f.end('fileExtension')]
        absFilePath = deviceLabel + filePath + fileName + '.' + fileExt
        print(absFilePath)
    return (deviceLabel, filePath, fileName, fileExt, absFilePath)

def __recursiveFolderProcess(folderPath, func):
    for file in scandir(folderPath):
        if file.is_dir():
            __recursiveFolderProcess(file, func)
        if file.is_file():
            func(file)

def __ifNoneEmptyStr(s):
    if s is None:
        return ''
    return s

# class File:
#     def __init__(self, name, path):
#         self.name = name
#         self.path = path
#         self.is_dir = False
#         self.children = []
#     def insert(self, f):
#         self.children.append(f)
    
#     def print(self, spaces = 0):
#         s = ' ' * spaces + ' - ' + self.name + ' (' + self.path + ')\n'
#         for p in self.children:
#             s += ' ' * spaces + p.print(spaces+2)
#         return s
        
#     def __str__(self):
#         return self.print()

#     def __repr__(self):
#         return str(self)

# def recursiveFileSearch(text: path):
#     folder = Folder(path)

def parseInputArguments(argsToParse, **opts):
    caseSensitive = True
    relaxedMode = False
    inputOptions = ()
    for key, value in opts.items():
        if key == 'case_sensitive':
            caseSensitive = value
        if key == 'admit_unknown_options':
            relaxedMode = value
        if key == 'opts':
            inputOptions = value
    options = {}
    for opt in inputOptions:
        if caseSensitive:
            key = opt[0]
        else:
            key = opt[0].lower()
        options[key] = opt[1]
    for arg in argsToParse:
        if caseSensitive:
            arg_adapted = arg
        else:
            arg_adapted = arg.lower()
        if arg_adapted not in options:
            if not relaxedMode:
                raise NameError('Unkown option specified.')
        else:
            options[arg_adapted] = argsToParse[arg]
    return (v for k, v in options.items())


# The following is an example on how to use parseInput function
# def importantFcn(a,**inputArgs):
#     opts = (('optionA', 33), 
#             ('optionB', 'lalala'),
#             ('optionC', [range(5)]))
#     (optionA, optionB, optionC) = parseInputArguments(inputArgs, opts = opts, admit_unknown_options = True, case_sensitive = True)
#     print(a)
#     print('OptionA : ' + str(optionA))
#     print('OptionB : ' + optionB)
#     print('OptionC : ' + str(optionC))

# importantFcn('5134',optionC = 'obladiriolaride', tetetetet = '321', opTIona = 37)