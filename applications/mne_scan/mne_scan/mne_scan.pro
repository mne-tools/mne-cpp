#==============================================================================================================
#
# @file     mne_scan.pro
# @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
# @version  dev
# @date     February, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Robert Dicamillo, Christoph Dinh, Lorenz Esch, Simon Heinke. All rights reserved.
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
# @brief    This project file builds the mne-scan main application.
#
#==============================================================================================================

include(../../../mne-cpp.pri)

TEMPLATE = app

QT += network core widgets xml svg charts concurrent

qtHaveModule(3dextras) {
    QT += 3dextras
}

contains(MNECPP_CONFIG, dispOpenGL) {
    qtHaveModule(opengl): QT += opengl
    DEFINES += USE_OPENGL
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
    # For static builds we need to link against the plugins
    # because we cannot load them dynamically during runtime
    LIBS += -L$${MNE_BINARY_DIR}/mne_scan_plugins
    LIBS += -lbabymeg \
            -lfiffsimulator \
            -lnatus \
            -lcovariance \
            -lnoisereduction \
            -lrtcmne \
            -laveraging \
            -lneuronalconnectivity \
            -lftbuffer \
            -lwritetofile \
            -lhpi
            #-ldummytoolbox

    contains(MNECPP_CONFIG, withGUSBAmp) {
        LIBS += -lgusbamp
        DEFINES += WITHGUSBAMP
    }
    contains(MNECPP_CONFIG, withBrainAmp) {
        LIBS += -lbrainamp
        DEFINES += WITHBRAINAMP
    }
    contains(MNECPP_CONFIG, withEego) {
        LIBS += -leegosports
        DEFINES += WITHEEGOSPORTS
    }
    contains(MNECPP_CONFIG, withLsl) {
        LIBS += -llsladapter
        DEFINES += WITHLSL
    }
    contains(MNECPP_CONFIG, withTmsi) {
        LIBS += -ltmsi
        DEFINES += WITHTMSI
    }
    contains(MNECPP_CONFIG, withBrainFlow) {
        LIBS += -lbrainflowboard
        DEFINES += WITHBRAINFLOW
    }
} else {
    CONFIG += shared
}

TARGET = mne_scan

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

CONFIG += console

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lscSharedd \
            -lscDispd \
            -lscMeasd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}RtProcessingd \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Communicationd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Utilsd \
} else {
    LIBS += -lscShared \
            -lscDisp \
            -lscMeas \
            -lMNE$${MNE_LIB_VERSION}Disp3D \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}RtProcessing \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Communication \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Utils \
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \
    startupwidget.cpp \
    mainsplashscreen.cpp \
    pluginscene.cpp \
    pluginitem.cpp \
    plugingui.cpp \
    arrow.cpp \
    mainwindow.cpp

HEADERS += \
    info.h \
    startupwidget.h \
    mainsplashscreen.h \
    pluginscene.h \
    pluginitem.h \
    plugingui.h \
    arrow.h \
    mainwindow.h

FORMS +=

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

RESOURCES += \
    mne_scan.qrc

unix: QMAKE_CXXFLAGS += -Wno-attributes

# Icon
win32 {
    RC_FILE = images/appIcons/mne_scan.rc
}
macx {
    ICON = images/appIcons/mne_scan.icns
}

# Deploy dependencies
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployAppArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${LIBS},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
unix:!macx {
    # Unix
    QMAKE_RPATHDIR += $ORIGIN/../lib
}
macx {
    # Mac
    QMAKE_RPATHDIR += @executable_path/../Frameworks

    # Copy Resource folder to app bundle
    filtrc.path = Contents/MacOS/resources/general/
    filtrc.files = $${ROOT_DIR}/resources/general/default_filters
    QMAKE_BUNDLE_DATA += filtrc

    sgrc.path = Contents/MacOS/resources/general/
    sgrc.files = $${ROOT_DIR}/resources/general/selectionGroups
    QMAKE_BUNDLE_DATA += sgrc

    loutrc.path = Contents/MacOS/resources/general/
    loutrc.files = $${ROOT_DIR}/resources/general/2DLayouts
    QMAKE_BUNDLE_DATA += loutrc

    hpirc.path = Contents/MacOS/resources/general/
    hpirc.files = $${ROOT_DIR}/resources/general/hpiAlignment
    QMAKE_BUNDLE_DATA += hpirc

    ssrc.path = Contents/MacOS/resources/general/
    ssrc.files = $${ROOT_DIR}/resources/general/sensorSurfaces
    QMAKE_BUNDLE_DATA += ssrc

    lout3rc.path = Contents/MacOS/resources/general/
    lout3rc.files = $${ROOT_DIR}/resources/general/3DLayouts
    QMAKE_BUNDLE_DATA += lout3rc

    rcplugins.path = Contents/MacOS/resources/mne_scan/
    rcplugins.files = $${ROOT_DIR}/resources/mne_scan/plugins
    QMAKE_BUNDLE_DATA += rcplugins

    plugins.path = Contents/MacOS/
    plugins.files = $${ROOT_DIR}/bin/mne_scan_plugins
    QMAKE_BUNDLE_DATA += plugins

    # 3 entries returned in DEPLOY_CMD
    DEPLOY_CMD = $$macDeployArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}

    QMAKE_CLEAN += -r $$member(DEPLOY_CMD, 1)
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
