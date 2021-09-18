#==============================================================================================================
#
# @file     lsladapter.pro
# @author   Lorenz Esch <lesch@mgh.harvard.edu>;
#           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
# @since    0.1.0
# @date     February, 2019
#
# @section  LICENSE
#
# Copyright (C) 2019, Lorenz Esch, Simon Heinke. All rights reserved.
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
# @brief    This project file generates the makefile for the LSL adapter plug-in.
#
#==============================================================================================================

include(../../../../mne-cpp.pri)

TEMPLATE = lib

QT += core widgets network concurrent

CONFIG += skip_target_version_ext plugin

DEFINES += LSLADAPTER_PLUGIN

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

TARGET = lsladapter
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lscSharedd \
            -lscDispd \
            -lscMeasd \
            -lmnecppDispd \
            -lmnecppEventsd \
            -lmnecppInversed \
            -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lscShared \
            -lscDisp \
            -lscMeas \
            -lmnecppDisp \
            -lmnecppEvents \
            -lmnecppInverse \
            -lmnecppFwd \
            -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

LIBS += -L"$$PWD/liblsl/build/install/lib/" -llsl

SOURCES += \
        lsladapter.cpp \
    lsladapter_global.cpp \
        lsladapterproducer.cpp \
        FormFiles/lsladaptersetup.cpp \

HEADERS += \
        lsladapter.h \
        lsladapter_global.h \
        lsladapterproducer.h \
        FormFiles/lsladaptersetup.h \

FORMS += \
        FormFiles/lsladaptersetup.ui \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}
INCLUDEPATH += $$PWD/liblsl/build/install/include

OTHER_FILES += lsladapter.json

unix:!macx {
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

################################################## BUILD TIMESTAMP/HASH UPDATER ############################################

FILETOUPDATE = lsladapter_global.cpp

ALLFILES += $$HEADERS
ALLFILES += $$SOURCES
ALLFILES -= $$FILETOUPDATE
FileUpdater.target = phonyFileUpdater
unix|macx {
    FileUpdater.commands = touch $$PWD/$$FILETOUPDATE ; echo PASTA > phonyFileUpdater
}
win32 {
    FileUpdater.commands = copy $$PWD/$$FILETOUPDATE +,, & echo PASTA > phonyFileUpdater
}
for (IFILE, ALLFILES) {
    FileUpdater.depends += $$PWD/$$IFILE
}
PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater

