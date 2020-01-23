#--------------------------------------------------------------------------------------------------------------
#
# @file     scShared.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @version  dev
# @date     March, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh, Lorenz Esch. All rights reserved.
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
# @brief    This project file builds the mne_x library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../../mne-cpp.pri)

TEMPLATE = lib

QT += widgets svg network
DEFINES += SCSHARED_LIBRARY

TARGET = scShared
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
    LIBS += -L$${MNE_BINARY_DIR}/mne_scan_plugins
    QTPLUGIN += ecgsimulator
    QTPLUGIN += fiffsimulator
    QTPLUGIN += neuromag
    QTPLUGIN += babymeg
    QTPLUGIN += triggercontrol
    QTPLUGIN += natus
#    QTPLUGIN += gusbamp
#    QTPLUGIN += eegosports
#    QTPLUGIN += brainamp
#    QTPLUGIN += tmsi
#    QTPLUGIN += lsladapter
    QTPLUGIN += dummytoolbox
    QTPLUGIN += epidetect
    QTPLUGIN += rtcmne
    QTPLUGIN += rtcmusic
    QTPLUGIN += averaging
    QTPLUGIN += covariance
    QTPLUGIN += noise
    QTPLUGIN += bci
    QTPLUGIN += rtsss
    QTPLUGIN += rthpi
    QTPLUGIN += noisereduction
    QTPLUGIN += rthpi
    QTPLUGIN += ssvepbci
    QTPLUGIN += neuronalconnectivity
    QTPLUGIN += reference
#    QTPLUGIN += ftbuffer
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lscMeasd \
            -lscDispd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lscMeas \
            -lscDisp
}

SOURCES += \
    Management/pluginmanager.cpp \
    Management/pluginconnector.cpp \
    Management/plugininputconnector.cpp \
    Management/pluginoutputconnector.cpp \
    Management/plugininputdata.cpp \
    Management/pluginoutputdata.cpp \
    Management/pluginconnectorconnection.cpp \
    Management/pluginconnectorconnectionwidget.cpp \
    Management/pluginscenemanager.cpp \
    Management/displaymanager.cpp

HEADERS += \
    scshared_global.h \
    Interfaces/IPlugin.h \
    Interfaces/ISensor.h \
    Interfaces/IAlgorithm.h \
    Interfaces/IIO.h \
    Management/pluginmanager.h \
    Management/pluginconnector.h \
    Management/plugininputconnector.h \
    Management/pluginoutputconnector.h \
    Management/plugininputdata.h \
    Management/pluginoutputdata.h \
    Management/pluginconnectorconnection.h \
    Management/pluginconnectorconnectionwidget.h \
    Management/pluginscenemanager.h \
    Management/displaymanager.h


INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/scShared

INSTALLS += header_files

# Deploy library in non-static builds only
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
unix:!macx {
    QMAKE_CXXFLAGS += -std=c++0x
    QMAKE_CXXFLAGS += -Wno-attributes
}
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
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

