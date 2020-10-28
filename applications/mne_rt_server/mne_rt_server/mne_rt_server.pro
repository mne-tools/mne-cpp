#==============================================================================================================
#
# @file     mne_rt_server.pro
# @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Limin Sun <limin.sun@childrens.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Robert Dicamillo, Christoph Dinh, Limin Sun, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
# @brief    This project file generates the makefile to build the core app.
#
#==============================================================================================================

include(../../../mne-cpp.pri)

TEMPLATE = app

QT += network concurrent
QT -= gui

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $${MNE_BINARY_DIR}

TARGET = mne_rt_server
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICBUILD
    LIBS += -L$${MNE_BINARY_DIR}/mne_rt_server_plugins
    LIBS += -lfiffsimulator
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppCommunicationd \
            -lmnecppFiffd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppCommunication \
            -lmnecppFiff \
            -lmnecppUtils \
}

SOURCES += \
    main.cpp \
    connectormanager.cpp \
    mne_rt_server.cpp \
    fiffstreamserver.cpp \
    fiffstreamthread.cpp \
    commandserver.cpp \
    commandthread.cpp

HEADERS += \
    IConnector.h \
    connectormanager.h \
    mne_rt_server.h \
    fiffstreamserver.h \
    fiffstreamthread.h \
    commandserver.h \
    commandthread.h \
    mne_rt_commands.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

macx {
    QMAKE_LFLAGS += -Wl,-rpath,../lib
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

