#==============================================================================================================
#
# @file     mne_scan.pro
# @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
# @since    0.1.0
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

QMAKE_TARGET_DESCRIPTION = MNE Scan

TEMPLATE = app

QT += network core widgets xml svg charts concurrent opengl 3dextras

DESTDIR = $${MNE_BINARY_DIR}

TARGET = mne_scan
CONFIG(debug, debug|release) {
    CONFIG += console
    TARGET = $$join(TARGET,,,d)
}

!contains(MNECPP_CONFIG, withAppBundles) {
    CONFIG -= app_bundle
}

contains(MNECPP_CONFIG, noQOpenGLWidget) {
    DEFINES += NO_QOPENGLWIDGET
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
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
	    -lhpi \
            -lrtfwd \
            -lneurofeedback \
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
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lscSharedd \
            -lscDispd \
            -lscMeasd \
            -lmnecppDisp3Dd \
            -lmnecppDispd \
            -lmnecppEventsd \
            -lmnecppRtProcessingd \
            -lmnecppConnectivityd \
            -lmnecppInversed \
            -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppCommunicationd \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lscShared \
            -lscDisp \
            -lscMeas \
            -lmnecppDisp3D \
            -lmnecppDisp \
            -lmnecppEvents \
            -lmnecppRtProcessing \
            -lmnecppConnectivity \
            -lmnecppInverse \
            -lmnecppFwd \
            -lmnecppMne \
            -lmnecppCommunication \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    main.cpp \
    mainsplashscreencloser.cpp \
    startupwidget.cpp \
    mainsplashscreen.cpp \
    pluginscene.cpp \
    pluginitem.cpp \
    plugingui.cpp \
    arrow.cpp \
    mainwindow.cpp

HEADERS += \
    info.h \
    mainsplashscreencloser.h \
    startupwidget.h \
    mainsplashscreen.h \
    pluginscene.h \
    pluginitem.h \
    plugingui.h \
    arrow.h \
    mainwindow.h

clang {
    QMAKE_CXXFLAGS += -isystem $${EIGEN_INCLUDE_DIR} 
} else {
    INCLUDEPATH += $${EIGEN_INCLUDE_DIR} 
}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

RESOURCES += \
    mne_scan.qrc \
    $${ROOT_DIR}/bin/resources/general/styles/styles.qrc \
    $${ROOT_DIR}/bin/resources/general/fonts/fonts.qrc

win32 {
    RC_FILE = images/appIcons/mne_scan.rc
}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

macx {
    ICON = images/appIcons/mne_scan.icns

    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/../lib
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
