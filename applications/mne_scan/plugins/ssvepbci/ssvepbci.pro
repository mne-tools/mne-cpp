#--------------------------------------------------------------------------------------------------------------
#
# @file     ssvepbci.pro
# @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>
#           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     May, 2016
#
# @section  LICENSE
#
# Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
# @brief    This project file generates the makefile for the ssvepbci-plugin.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += SSVEPBCI_LIBRARY

QT += core widgets concurrent gui

TARGET = SSVEPBCI
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lscMeas \
            -lscDisp \
            -lscShared
}

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

SOURCES += \
        ssvepbci.cpp \
        FormFiles/ssvepbciwidget.cpp \
        FormFiles/ssvepbciaboutwidget.cpp \
        FormFiles/ssvepbcisetupstimuluswidget.cpp \
        ssvepbciscreen.cpp \
        ssvepbciflickeringitem.cpp \
        FormFiles/ssvepbciconfigurationwidget.cpp \
        screenkeyboard.cpp \

HEADERS += \
        ssvepbci.h\
        ssvepbci_global.h \
        FormFiles/ssvepbciwidget.h \
        FormFiles/ssvepbciaboutwidget.h \
        FormFiles/ssvepbcisetupstimuluswidget.h \
        ssvepbciscreen.h \
        ssvepbciflickeringitem.h \
        FormFiles/ssvepbciconfigurationwidget.h \
        screenkeyboard.h \


FORMS += \
        FormFiles/ssvepbciwidget.ui \
        FormFiles/ssvepbcisetupstimuluswidget.ui \
        FormFiles/ssvepbciaboutwidget.ui \
        FormFiles/ssvepbciconfigurationwidget.ui \

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
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
}

OTHER_FILES += ssvepbci.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}

RESOURCES += \
        ssvepbci.qrc

DISTFILES +=
