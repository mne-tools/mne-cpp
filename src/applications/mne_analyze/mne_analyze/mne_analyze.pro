#==============================================================================================================
#
# @file     mne_analyze.pro
# @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Erik Hornberger <erik.hornberger@shi-g.com>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @since    0.1.0
# @date     January, 2017
#
# @section  LICENSE
#
# Copyright (C) 2017, Robert Dicamillo, Christoph Dinh, Erik Hornberger, Lorenz Esch. All rights reserved.
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
# @brief    This project file builds the MNE Analyze project
#
#==============================================================================================================

include(../../../mne-cpp.pri)

QMAKE_TARGET_DESCRIPTION = MNE Analyze

TEMPLATE = app

QT += gui widgets network opengl svg concurrent charts
qtHaveModule(printsupport): QT += printsupport

DESTDIR = $${MNE_BINARY_DIR}

TARGET = mne_analyze
CONFIG(debug, debug|release) {
    CONFIG += console
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, noQOpenGLWidget) {
    DEFINES += NO_QOPENGLWIDGET
}

!contains(MNECPP_CONFIG, withAppBundles) {
    CONFIG -= app_bundle
}

contains(MNECPP_CONFIG, wasm) {
#    QMAKE_LFLAGS += -s ERROR_ON_UNDEFINED_SYMBOLS=1
#    QMAKE_LFLAGS += -s ASSERTIONS=1
#    QMAKE_LFLAGS += -s STRICT=0
#    QMAKE_LFLAGS += -s FORCE_FILESYSTEM=1

    DEFINES += __EMSCRIPTEN__
    LIBS += -lidbfs.js
    INCLUDEPATH += /home/lorenz/Git/emsdk/usptream/emscripten/src

    DEFINES += WASMBUILD
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICBUILD
    # For static builds we need to link against the plugins
    # because we cannot load them dynamically during runtime
    LIBS += -L$${MNE_BINARY_DIR}/mne_analyze_plugins
    LIBS += -ldataloader \
            -ldatamanager \
            -lrawdataviewer \
            -levents \
            -lfiltering \
            -laveraging \
            -lsourcelocalization \
            -lcontrolmanager \
            -lchannelselection \
	    -lcoregistration \

    # Add Qt3D/Disp3D based plugins only if not building against WASM, which does not support Qt3D
    !contains(DEFINES, WASMBUILD) {
        LIBS += -lview3d \
    }            
}

LIBS += -L$${MNE_LIBRARY_DIR}

# Link Disp3D library only if not building against WASM, which does not support Qt3D
!contains(DEFINES, WASMBUILD) {
   QT += 3dextras

   CONFIG(debug, debug|release) {
       LIBS += -lmnecppDisp3Dd \
   } else {
       LIBS += -lmnecppDisp3D \
   }
}

CONFIG(debug, debug|release) {
    LIBS += -lanSharedd \
            -lmnecppDispd \
            -lmnecppEventsd \
            -lmnecppConnectivityd \
            -lmnecppRtProcessingd \
            -lmnecppInversed \
            -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lanShared \
            -lmnecppDisp \
            -lmnecppEvents \
            -lmnecppConnectivity \
            -lmnecppRtProcessing \
            -lmnecppInverse \
            -lmnecppFwd \
            -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    analyzecore.cpp \

HEADERS += \
    info.h \
    mainwindow.h \
    analyzecore.h \

RESOURCES += \
        mne_analyze.qrc \
        $${ROOT_DIR}/bin/resources/general/styles/styles.qrc \
        $${ROOT_DIR}/bin/resources/general/fonts/fonts.qrc

clang {
    QMAKE_CXXFLAGS += -isystem $${EIGEN_INCLUDE_DIR} 
} else {
    INCLUDEPATH += $${EIGEN_INCLUDE_DIR} 
}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_ANALYZE_INCLUDE_DIR}

win32 {
    RC_FILE = resources/images/appIcons/mne_analyze.rc
}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

macx {
    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/../lib

    ICON = resources/images/appIcons/mne_analyze.icns
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
