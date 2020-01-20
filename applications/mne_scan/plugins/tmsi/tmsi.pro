#--------------------------------------------------------------------------------------------------------------
#
# @file     tmsi.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @version  dev
# @date     September, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
# @brief    This project file generates the makefile for the tmsi plug-in.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += TMSI_LIBRARY

contains(QMAKE_HOST.arch, x86_64) { #Compiling MNE-X FOR a 64bit system
    exists(C:/Windows/System32/TMSiSDK.dll) {
        DEFINES += TAKE_TMSISDK_DLL
    }
}
else {
    exists(C:/Windows/SysWOW64/TMSiSDK32bit.dll) { #Compiling MNE-X FOR a 32bit system ON a 64bit system
        DEFINES += TAKE_TMSISDK_32_DLL
    }
    else {
        exists(C:/Windows/System32/TMSiSDK.dll) { #Compiling MNE-X FOR a 32bit system ON a 32bit system
            DEFINES += TAKE_TMSISDK_DLL
        }
        else {
            message(TMSI.pro warning: TMSi Driver DLL not found!)
        }
    }
}

QT += core widgets svg

TARGET = tmsi
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
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lscMeas \
            -lscDisp \
            -lscShared
}

SOURCES += \
        tmsi.cpp \
        tmsiproducer.cpp \
        FormFiles/tmsisetupwidget.cpp \
        FormFiles/tmsiaboutwidget.cpp \
        tmsidriver.cpp \
        FormFiles/tmsimanualannotationwidget.cpp \
        FormFiles/tmsiimpedancewidget.cpp \
        tmsielectrodeitem.cpp \
        tmsiimpedanceview.cpp \
        tmsiimpedancescene.cpp \
        FormFiles/tmsisetupprojectwidget.cpp

HEADERS += \
        tmsi.h\
        tmsi_global.h \
        tmsiproducer.h \
        FormFiles/tmsisetupwidget.h \
        FormFiles/tmsiaboutwidget.h \
        tmsidriver.h \
        FormFiles/tmsimanualannotationwidget.h \
        FormFiles/tmsiimpedancewidget.h \
        tmsielectrodeitem.h \
        tmsiimpedanceview.h \
        tmsiimpedancescene.h \
        FormFiles/tmsisetupprojectwidget.h

FORMS += \
        FormFiles/tmsisetup.ui \
        FormFiles/tmsiabout.ui \
        FormFiles/tmsimanualannotation.ui \
        FormFiles/tmsiimpedancewidget.ui \
        FormFiles/tmsisetupprojectwidget.ui

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/readme.txt \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/Lorenz-Duke128-28-11-2013.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard_waveguard8.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard_waveguard32.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard_waveguard64.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard_waveguard128.elc \
    $${ROOT_DIR}/resources/mne_scan/plugins/tmsi/loc_files/standard_waveguard256.elc \

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

OTHER_FILES += tmsi.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# suppress visibility warnings
unix: QMAKE_CXXFLAGS += -Wno-attributes

RESOURCES += \
    tmsi.qrc

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
