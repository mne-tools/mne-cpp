#--------------------------------------------------------------------------------------------------------------
#
# @file     babymeg.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @version  dev
# @date     November, 2016
#
# @section  LICENSE
#
# Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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
# @brief    This project file generates the makefile for the babymeg plug-in.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += BABYMEG_LIBRARY

QT += widgets 3dextras network

TARGET = babymeg
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
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Communicationd \
            -lMNE$${MNE_LIB_VERSION}RtProcessingd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Communication \
            -lMNE$${MNE_LIB_VERSION}RtProcessing \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}Disp3D \
            -lscMeas \
            -lscDisp \
            -lscShared
}

SOURCES += \
    babymeg.cpp \
    babymegclient.cpp \
    babymeginfo.cpp \
    FormFiles/babymegsetupwidget.cpp \
    FormFiles/babymegaboutwidget.cpp \
    FormFiles/babymegsquidcontroldgl.cpp \
    FormFiles/globalobj.cpp \
    FormFiles/plotter.cpp \

HEADERS += \
    babymeg.h \
    babymegclient.h \
    babymeginfo.h \
    babymeg_global.h \
    FormFiles/babymegsetupwidget.h \
    FormFiles/babymegaboutwidget.h \
    FormFiles/babymegsquidcontroldgl.h \
    FormFiles/globalobj.h \
    FormFiles/plotter.h \

FORMS += \
    FormFiles/babymegsetup.ui \
    FormFiles/babymegabout.ui \
    FormFiles/babymegsquidcontroldgl.ui \

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/mne_scan/plugins/babymeg/both.bad \
    $${ROOT_DIR}/resources/mne_scan/plugins/babymeg/header.fif \
    $${ROOT_DIR}/resources/mne_scan/plugins/babymeg/readme.txt \

# Copy resource files from repository to bin resource folder
COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

RESOURCES += \
    babymeg.qrc

OTHER_FILES += babymeg.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# suppress visibility warnings
unix: QMAKE_CXXFLAGS += -Wno-attributes

unix:!macx {
    # === Unix ===
    QMAKE_RPATHDIR += $ORIGIN/../../lib
}
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
