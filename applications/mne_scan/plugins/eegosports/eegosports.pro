#==============================================================================================================
#
# @file     eegosports.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
#           Johannes Vorwerk <johannes.vorwerk@umit.at>
# @since    0.1.0
# @date     February, 2020
#
# @section  LICENSE
#
# Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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
#==============================================================================================================

include(../../../../mne-cpp.pri)

TEMPLATE = lib

QT += core widgets svg

CONFIG += skip_target_version_ext

CONFIG += plugin

DEFINES += EEGOSPORTS_LIBRARY

DESTDIR = $${MNE_BINARY_DIR}/mne_scan_plugins

TARGET = eegosports
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
            -lmnecppFiffd \
            -lmnecppUtilsd \
} else {
    LIBS += -lscShared \
            -lscDisp \
            -lscMeas \
            -lmnecppDisp \
            -lmnecppEvents \
            -lmnecppFiff \
            -lmnecppUtils \
}

SOURCES += \
        FormFiles/eegosportsimpedancewidget.cpp \
        eegosports.cpp \
    eegosports_global.cpp \
        eegosportselectrodeitem.cpp \
        eegosportsimpedancescene.cpp \
        eegosportsimpedanceview.cpp \
        eegosportsproducer.cpp \
        FormFiles/eegosportssetupwidget.cpp \
        eegosportsdriver.cpp \
        FormFiles/eegosportssetupprojectwidget.cpp \

HEADERS += \
        FormFiles/eegosportsimpedancewidget.h \
        eegosports.h\
        eegosports_global.h \
        eegosportselectrodeitem.h \
        eegosportsimpedancescene.h \
        eegosportsimpedanceview.h \
        eegosportsproducer.h \
        FormFiles/eegosportssetupwidget.h \
        eegosportsdriver.h \
        FormFiles/eegosportssetupprojectwidget.h \

FORMS += \
        FormFiles/eegosportsimpedancewidget.ui \
        FormFiles/eegosportssetup.ui \
        FormFiles/eegosportssetupprojectwidget.ui \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

OTHER_FILES += eegosports.json

RESOURCES += \
    eegosports.qrc

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

FILE_TO_UPDATE = eegosports_global.cpp
win32 {
    CONFIG(debug, debug|release) {
        OBJ_TARJET = debug\eegosports_global.obj
    } else {
        OBJ_TARJET = release\eegosports_global.obj
    }
}

ALL_FILES += $HEADERS
ALL_FILES += $SOURCES
ALL_FILES -= $FILE_TO_UPDATE

FileUpdater.target = phonyFileUpdater
for (I_FILE, ALL_FILES) {
    FileUpdater.depends += ${PWD}/${I_FILE}
}

unix|macx {
    FileUpdater.commands = touch ${PWD}/${FILE_TO_UPDATE} ; echo PASTA > phonyFileUpdater
}

win32 {
    FileUpdater.commands = copy /y $shell_path(${PWD})\${FILE_TO_UPDATE} +,, $shell_path(${PWD})\${FILE_TO_UPDATE} & echo PASTA > phonyFileUpdater
    OrderForcerTarget.target = ${OBJ_TARJET}
    OrderForcerTarget.depends += phonyFileUpdater
    QMAKE_EXTRA_TARGETS += OrderForcerTarget
}

PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater