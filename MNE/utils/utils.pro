#--------------------------------------------------------------------------------------------------------------
#
# @file     utils.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
# @brief    This project file builds the Utils library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT       -= gui
QT       += xml

DEFINES += UTILS_LIBRARY

TARGET = Utils
TARGET = $$join(TARGET,,MNE$$MNE_LIB_VERSION,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_LIBRARY_DIR}

#
# win32: copy dll's to bin dir
# unix: add lib folder to LD_LIBRARY_PATH
#
win32 {
    FILE = $${DESTDIR}/$${TARGET}.dll
    BINDIR = $${DESTDIR}/../bin
    FILE ~= s,/,\\,g
    BINDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${BINDIR}) $$escape_expand(\\n\\t)
}

SOURCES += \
    kmeans.cpp \
    mnemath.cpp \
    ioutils.cpp \
    layoutloader.cpp \
    parksmcclellan.cpp \
    filterdata.cpp \
    mp/adaptivemp.cpp \
    mp/atom.cpp \
    mp/fixdictmp.cpp

HEADERS += \
    kmeans.h\
    utils_global.h \
    mnemath.h \
    ioutils.h \
    layoutloader.h \
    parksmcclellan.h \
    filterdata.h \
    mp/adaptivemp.h \
    mp/atom.h \
    mp/fixdictmp.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/utils

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR
