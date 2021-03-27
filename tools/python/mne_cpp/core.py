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

# This file should be kept inside the following folder: mne_base_folder/tools/python/mne_cpp

def version():
    print('MNE-CPP Project python module. - Version: 0.1.9')
    print('Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.')

def _currentPath():
    return path.abspath(path.dirname(sys.argv[0]))


def base_folder():
    file_path_list = _currentPath().split(path.sep)
    project_path = ''
    for f in file_path_list[:-2]:
        project_path += f + '/'
    return project_path

class File_ext_separator(Enum):
    FIRST = 1
    LAST = 2

def extract_filepaths(text: str, **inputArgs):
    dot_extension_sep = 'last'
    for key, value in inputArgs
        if key.lower() == 'dot_extension_separator':
            dot_extension_sep = value.lower()
#(?P<deviceLabel>[A-Za-z]:)?(?P<filePath>([/\\]?)(?P<lastFolder>(\.{1,2}|[^\n.<>:"|?*/\\]+)[/\\])+)(?P<fileName>([^/\\{}\n?*()":\' ][^/\\{}()\n?*":\'][^/\\{}()\n?*":\' .]*)+)(\.?(?=.))(?P<fileExtension>[A-Za-z0-9_.]*)
    if dot_extension_sep == 'first':    # first '.' in filename separates ext
        expression = re.compile(r"""
                                (?P<deviceLabel>[A-Za-z]:)?     # Device Label. c:  D: etc... for windows.
                                (?P<filePath>([/\\]?)           # filePath: absolute or relative path. /folder/file.txt vs folder/file.txt
                                                                #                                      ^                   ^         
                                 (?P<lastFolder>(\.{1,2}|[^\n.<>:"|?*/\\]+)[/\\])+  # An undefined number of nested folders '.' and '..' admitted. 
                                                                                    # The forbidden char set is mostly due to windows = [^\n.<>:"|?*/\\] but 
                                                                                    # using them in linux is kind of asking for trouble. So out!
                                )                                                                                    
                                (?P<fileName>([^/\\{}\n?*":\' ][^/\\{}\n?*":\'][^/\\{}\n?*":\' .]*?)+?)  #
                                \.       #
                                (?P<fileExtension>[A-Za-z0-9_.]*)                  
                                """,re.X)
    elif dot_extension_sep == 'last':
        expression = re.compile(r'(?P<deviceLabel>[A-Za-z]:)?(?P<filePath>([/\\]?)(?P<lastFolder>(\.{1,2}|[^.<>:"|?*/\\]+)[/\\])+)(?P<fileName>[A-Za-z_0-9^.]+)\.(?P<fileExtension>[A-Za-z0-9_.]*)')    # last '.' in filename separates ext (filename not greedy)
    else: 
        return []
    fileList = []
    pattern = re.compile(expression)
    fileMatches = pattern.finditer(text)
    for f in fileMatches:
        deviceLabel = f.string[f.start('deviceLabel'):f.end('deviceLabel')]
        filePath = f.string[f.start('filePath'):f.end('filePath')-1]
        fileName = f.string[f.start('fileName'):f.end('fileName')]
        fileExt = f.string[f.start('fileExtension'):f.end('fileExtension')]
        absFilePath = deviceLabel + '/' + filePath + '/' + fileName + '.' + fileExt
        print(absFilePath)
        fileList.append((deviceLabel, filePath, fileName, fileExt, absFilePath))
    return fileList

def _recursiveFolderProcess(folderPath, func):
    for file in scandir(folderPath):
        if file.is_dir():
            _recursiveFolderProcess(file, func)
        if file.is_file():
            func(file)




