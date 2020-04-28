#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_analyze.pro
# @author   Robert Dicamillo <rd521@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Erik Hornberger <erik.hornberger@shi-g.com>;
#           Lorenz Esch <lesch@mgh.harvard.edu>
# @version  dev
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
#--------------------------------------------------------------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = app

QT += gui widgets network
qtHaveModule(printsupport): QT += printsupport

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
    DEFINES += STATICLIB
    LIBS += -L$${MNE_BINARY_DIR}/mne_analyze_extensions
    QTPLUGIN += dataloader
    QTPLUGIN += datamanager
    QTPLUGIN += rawdataviewer
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}RtProcessingd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lanSharedd
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}RtProcessing \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lanShared
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mdiview.cpp \
    analyzecore.cpp

HEADERS += \
    info.h \
    mainwindow.h \
    mdiview.h \
    analyzecore.h

FORMS +=

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${CNTK_INCLUDE_DIR}
INCLUDEPATH += $${MNE_ANALYZE_INCLUDE_DIR}

unix:!macx {
    QMAKE_CXXFLAGS += -std=c++0x
    QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

    # suppress visibility warnings
    QMAKE_CXXFLAGS += -Wno-attributes
}
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    CONFIG +=c++11
}

RESOURCES += \
    mne_analyze.qrc

# Icon
win32 {
    RC_FILE = resources/images/appIcons/mne_analyze.rc
}
macx {
    ICON = resources/images/appIcons/mne_analyze.icns
}

# Deploy dependencies
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployAppArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${LIBS},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
unix:!macx {
    # === Unix ===
    QMAKE_RPATHDIR += $ORIGIN/../lib
}
macx {
    # === Mac ===
    QMAKE_RPATHDIR += @executable_path/../Frameworks

    extensions.path = Contents/MacOS/
    extensions.files = $${ROOT_DIR}/applications/mne_analyze/extensions
    QMAKE_BUNDLE_DATA += extensions
    EXTRA_ARGS = -dmg

    # 3 entries returned in DEPLOY_CMD
    DEPLOY_CMD = $$macDeployArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}

    deploy_app = $$member(DEPLOY_CMD, 1)
    dmg_file = $$replace(deploy_app, .app, .dmg)
    QMAKE_CLEAN += -r $${deploy_app} $${dmg_file}
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
