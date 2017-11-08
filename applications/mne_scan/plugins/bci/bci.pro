#--------------------------------------------------------------------------------------------------------------
#
# @file     bci.pro
# @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     December, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
# @brief    This project file generates the makefile for the bci plug-in.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += BCI_LIBRARY

QT += core widgets concurrent gui

TARGET = bci
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lxMeasd \
            -lxDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lxMeas \
            -lxDisp \
            -lscShared
}

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

SOURCES += \
        bci.cpp \
        FormFiles/bcisetupwidget.cpp \
        FormFiles/bciaboutwidget.cpp \ 
        FormFiles/bcifeaturewindow.cpp

HEADERS += \
        bci.h\
        bci_global.h \
        FormFiles/bcisetupwidget.h \
        FormFiles/bciaboutwidget.h \  
        FormFiles/bcifeaturewindow.h

FORMS += \
        FormFiles/bcisetup.ui \
        FormFiles/bciabout.ui \
        FormFiles/bcifeaturewindow.ui

RESOURCE_FILES +=\
    $${MNE_DIR}/resources/mne_scan/plugins/bci/LDA_linear_boundary_Sensor.txt \
    $${MNE_DIR}/resources/mne_scan/plugins/bci/Pinning_Scheme_Duke_128.txt \
    $${MNE_DIR}/resources/mne_scan/plugins/bci/readme.txt \

# Copy resource files to bin resource folder
for(FILE, RESOURCE_FILES) {
    FILEDIR = $$dirname(FILE)
    FILEDIR ~= s,/resources,/bin/resources,g
    FILEDIR ~= s,/,\\,g
    TRGTDIR = $${FILEDIR}

    QMAKE_POST_LINK += $$sprintf($${QMAKE_MKDIR_CMD}, "$${TRGTDIR}") $$escape_expand(\n\t)

    FILE ~= s,/,\\,g
    TRGTDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${TRGTDIR}) $$escape_expand(\\n\\t)
}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

unix:!macx {
    QMAKE_CXXFLAGS += -std=c++0x
    QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

    # suppress visibility warnings
    QMAKE_CXXFLAGS += -Wno-attributes
}
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc+
    CONFIG +=c++11
}

OTHER_FILES += bci.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}
