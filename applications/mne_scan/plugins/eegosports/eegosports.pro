#--------------------------------------------------------------------------------------------------------------
#
# @file     eegosports.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
# @version  dev
# @date     July, 2014
#
# @section  LICENSE
#
# Copyright (C) 2014, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
# @brief    This project file generates the makefile for the eegosports plug-in.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += EEGOSPORTS_LIBRARY

QT += core widgets svg

TARGET = eegosports
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lscMeas \
            -lscDisp \
            -lscShared
}

SOURCES += \
        eegosports.cpp \
        eegosportsproducer.cpp \
        FormFiles/eegosportssetupwidget.cpp \
        FormFiles/eegosportsaboutwidget.cpp \
        eegosportsdriver.cpp \
        FormFiles/eegosportssetupprojectwidget.cpp \

HEADERS += \
        eegosports.h\
        eegosports_global.h \
        eegosportsproducer.h \
        FormFiles/eegosportssetupwidget.h \
        FormFiles/eegosportsaboutwidget.h \
        eegosportsdriver.h \
        FormFiles/eegosportssetupprojectwidget.h \

FORMS += \
        FormFiles/eegosportssetup.ui \
        FormFiles/eegosportsabout.ui \
        FormFiles/eegosportssetupprojectwidget.ui \

RESOURCE_FILES +=\
    $${MNE_DIR}/resources/mne_scan/plugins/eegosports/readme.txt \

# Copy resource files to bin resource folder
for(FILE, RESOURCE_FILES) {
    FILEDIR = $$dirname(FILE)
    FILEDIR ~= s,/resources,/bin/resources,g
    FILEDIR = $$shell_path($${FILEDIR})
    TRGTDIR = $${FILEDIR}

    QMAKE_POST_LINK += $$sprintf($${QMAKE_MKDIR_CMD}, "$${TRGTDIR}") $$escape_expand(\n\t)

    FILE = $$shell_path($${FILE})
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${TRGTDIR}) $$escape_expand(\\n\\t)
}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

OTHER_FILES += eegosports.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# suppress visibility warnings
unix: QMAKE_CXXFLAGS += -Wno-attributes

RESOURCES += \
    eegosports.qrc

DISTFILES +=

# Activate FFTW backend in Eigen for non-static builds only
contains(MNECPP_CONFIG, useFFTW):!contains(MNECPP_CONFIG, static) {
    DEFINES += EIGEN_FFTW_DEFAULT
    INCLUDEPATH += $$shell_path($${FFTW_DIR_INCLUDE})
    LIBS += -L$$shell_path($${FFTW_DIR_LIBS})

    win32 {
        # On Windows
        LIBS += -llibfftw3-3 \
                -llibfftw3f-3 \
                -llibfftw3l-3 \
    }

    unix:!macx {
        # On Linux
        LIBS += -lfftw3 \
                -lfftw3_threads \
    }
}
