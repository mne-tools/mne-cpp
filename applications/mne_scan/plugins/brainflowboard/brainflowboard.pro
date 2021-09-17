#==============================================================================================================
#
# @file     brainflowboard.pro
# @author   Andrey Parfenov <a1994ndrey@gmail.com>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @since    0.1.0
# @date     February, 2020
#
# @section  LICENSE
#
# Copyright (C) 2020, Andrey Parfenov, Lorenz Esch. All rights reserved.
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
# @brief    This project file generates the makefile for the brainflowboard plug-in.
#
#==============================================================================================================

include(../../../../mne-cpp.pri)

TEMPLATE = lib

QT += core widgets

CONFIG += skip_target_version_ext

CONFIG += c++11 plugin

DEFINES += BRAINFLOWBOARD_PLUGIN

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

TARGET = brainflowboard
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
            -lmnecppFiffd \
            -lmnecppUtilsd \
} else {
    LIBS += -lscShared \
            -lscDisp \
            -lscMeas \
            -lmnecppFiff \
            -lmnecppUtils \
}

contains(QT_ARCH, i386) {
    LIBS += -L"$$PWD/brainflow/installed/lib/" -lBrainflow32  -lDataHandler32 -lBoardController32
} else {
    LIBS += -L"$$PWD/brainflow/installed/lib/" -lBrainflow  -lDataHandler -lBoardController
}

INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}
INCLUDEPATH += $$PWD/brainflow/installed/inc
DEPENDPATH += $$PWD/brainflow/installed/inc

SOURCES += \
    FormFiles/brainflowsetupwidget.cpp \
    FormFiles/brainflowstreamingwidget.cpp \
    brainflowboard.cpp \
    brainflowboard_global.cpp

HEADERS += \
    FormFiles/brainflowsetupwidget.h \
    FormFiles/brainflowstreamingwidget.h \
    brainflowboard_global.h \
    brainflowboard.h

FORMS += \
    FormFiles/brainflowsetupwidget.ui \
    FormFiles/brainflowstreamingwidget.ui

DISTFILES += \
    brainflowboard.json

OTHER_FILES += \
    brainflowboard.json

unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

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

FILETOUPDATE = brainflowboard_global.cpp

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
FileUpdater.depends +=
for (IFILE, ALLFILES) {
    FileUpdater.depends += $$PWD/$$IFILE
}
PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater

