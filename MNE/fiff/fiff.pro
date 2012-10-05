#--------------------------------------------------------------------------------------------------------------
#
# @file     fiff.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds the fiff library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT       -= gui

DEFINES += FIFF_LIBRARY

TARGET = fiff

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

win32:QMAKE_POST_LINK += $${QMAKE_COPY} "..\\..\\..\\mne-cpp\\lib\\$${TARGET}.dll" "..\\..\\..\\mne-cpp\\bin\\"
DESTDIR = $${PWD}/../../lib

SOURCES += fiff.cpp \
#    src/fiff_parser.cpp \
    src/fiff_tag.cpp \
    src/fiff_dir_tree.cpp \
    src/fiff_coord_trans.cpp \
    src/fiff_ch_info.cpp \
    src/fiff_proj.cpp \
    src/fiff_named_matrix.cpp \
    src/fiff_file.cpp \
    src/fiff_raw_data.cpp \
    src/fiff_ctf_comp.cpp \
    src/fiff_id.cpp \
    src/fiff_info.cpp \
    src/fiff_raw_dir.cpp \
    src/fiff_dig_point.cpp \
    src/fiff_ch_pos.cpp

HEADERS += fiff.h \
        fiff_global.h \
    include/fiff_types.h \
    include/fiff_id.h \
    include/fiff_constants.h \
    include/fiff_tag.h \
    include/fiff_dir_tree.h \
    include/fiff_coord_trans.h \
    include/fiff_ch_info.h \
    include/fiff_proj.h \
    include/fiff_named_matrix.h \
    include/fiff_ctf_comp.h \
    include/fiff_info.h \
    include/fiff_raw_data.h \
    include/fiff_dir_entry.h \
    include/fiff_raw_dir.h \
    include/fiff_file.h \
    include/fiff_dig_point.h \
    include/fiff_ch_pos.h

#Install headers to include directory
baseheader_files.files = ./*.h
baseheader_files.path = ../../include/fiff
header_files.files = ./include/*.h
header_files.path = ../../include/fiff/include

INSTALLS += baseheader_files \
            header_files
