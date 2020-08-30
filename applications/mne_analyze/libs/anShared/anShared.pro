#==============================================================================================================
#
# @file     anShared.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @since    0.1.0
# @date     March, 2017
#
# @section  LICENSE
#
# Copyright (C) 2017, Christoph Dinh, Lorenz Esch. All rights reserved.
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
#==============================================================================================================

include(../../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += widgets svg

DEFINES += ANSHARED_LIBRARY

TARGET = anShared
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}
DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, wasm) {
    DEFINES += WASMBUILD
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppDispd \
            -lmnecppRtProcessingd \
            -lmnecppInversed \
            -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppDisp \
            -lmnecppRtProcessing \
            -lmnecppInverse \
            -lmnecppFwd \
            -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    Management/analyzedata.cpp \
    Management/analyzesettings.cpp \
    Management/pluginmanager.cpp \
    Management/event.cpp \
    Management/communicator.cpp \
    Management/eventmanager.cpp \
    Management/statusbar.cpp \
    Model/bemdatamodel.cpp \
    Model/fiffrawviewmodel.cpp \
    Model/annotationmodel.cpp \
    Model/analyzedatamodel.cpp \
    Model/averagingdatamodel.cpp \

HEADERS += \
    Model/bemdatamodel.h \
    anshared_global.h \
    Model/abstractmodel.h \
    Management/analyzedata.h \
    Management/analyzesettings.h \
    Management/pluginmanager.h \
    Management/event.h \
    Management/communicator.h \
    Management/eventmanager.h \
    Management/statusbar.h \
    Interfaces/IStandardView.h \
    Interfaces/IPlugin.h \
    Utils/metatypes.h \
    Utils/types.h \
    Model/fiffrawviewmodel.h \
    Model/annotationmodel.h \
    Model/analyzedatamodel.h \
    Model/averagingdatamodel.h \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_ANALYZE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/anShared

INSTALLS += header_files



DISTFILES += \
    Model/fiffrawmodel

win32:!contains(MNECPP_CONFIG, static) {
# Deploy/Copy library to bin folder manually (windeployqt only takes care of qt and system libraries)
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
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
