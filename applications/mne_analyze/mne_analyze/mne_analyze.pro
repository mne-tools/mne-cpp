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

TEMPLATE = app

QT += gui widgets network opengl svg concurrent
qtHaveModule(printsupport): QT += printsupport

contains(MNECPP_CONFIG, noQOpenGLWidget) {
    DEFINES += NO_QOPENGLWIDGET
}

TARGET = mne_analyze

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

CONFIG += console

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

DESTDIR = $${MNE_BINARY_DIR}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICBUILD
    # For static builds we need to link against the plugins
    # because we cannot load them dynamically during runtime
    LIBS += -L$${MNE_BINARY_DIR}/mne_analyze_plugins
    LIBS += -ldataloader \
            -ldatamanager \
            -lrawdataviewer \
            -lannotationmanager \
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

FORMS += \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_ANALYZE_INCLUDE_DIR}

unix:!macx {
    QMAKE_CXXFLAGS += -std=c++0x

    # suppress visibility warnings
    QMAKE_CXXFLAGS += -Wno-attributes
}
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
}

RESOURCES += \
        mne_analyze.qrc \
        $${ROOT_DIR}/resources/general/styles/styles.qrc \
        $${ROOT_DIR}/resources/general/fonts/fonts.qrc

# Icon
win32 {
    RC_FILE = resources/images/appIcons/mne_analyze.rc
}
macx {
    ICON = resources/images/appIcons/mne_analyze.icns
}

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $$PWD/formfiles

# Deploy dependencies
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployAppArgs($${TARGET},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}
macx {
    QMAKE_LFLAGS += -Wl,-rpath,../lib

    # Copy Resource and plugins folder to app bundle
    plugins.path = Contents/MacOS/
    plugins.files = $${ROOT_DIR}/bin/mne_analyze_plugins
    QMAKE_BUNDLE_DATA += plugins

    sgrc.path = Contents/MacOS/resources/general/
    sgrc.files = $${ROOT_DIR}/resources/general/selectionGroups
    QMAKE_BUNDLE_DATA += sgrc

    loutrc.path = Contents/MacOS/resources/general/
    loutrc.files = $${ROOT_DIR}/resources/general/2DLayouts
    QMAKE_BUNDLE_DATA += loutrc

    # If Qt3D plugins/renderers folder exisits, create and copy renderers folder to mne-cpp/bin manually.
    # macdeployqt does not deploy them. This will be fixed in Qt 5.15.2.
    exists($$shell_path($$[QT_INSTALL_PLUGINS]/renderers)) {
        qt3drenderers.path = Contents/PlugIns/
        qt3drenderers.files = $$[QT_INSTALL_PLUGINS]/renderers
        QMAKE_BUNDLE_DATA += qt3drenderers
    }

    !contains(MNECPP_CONFIG, static) {
        # 3 entries returned in DEPLOY_CMD
        EXTRA_ARGS =
        DEPLOY_CMD = $$macDeployArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
        QMAKE_POST_LINK += $${DEPLOY_CMD}
    }
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
