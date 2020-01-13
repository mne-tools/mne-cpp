#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_browse.pro
# @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
#           Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
# @version  1.0
# @date     January, 2014
#
# @section  LICENSE
#
# Copyright (C) 2014, Lorenz Esch, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
# @brief    This project file builds the mne_browse project
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT += gui network core widgets concurrent svg opengl

contains(MNECPP_CONFIG, dispOpenGL) {
    DEFINES += USE_OPENGL
}

TARGET = mne_browse

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

CONFIG += console

contains(MNECPP_CONFIG, wasm) {
    QMAKE_LFLAGS += -s ERROR_ON_UNDEFINED_SYMBOLS=1
    QMAKE_LFLAGS += -s ASSERTIONS=1

#    DEFINES += __EMSCRIPTEN__
#    LIBS +=-lidbfs.js
#    INCLUDEPATH += /home/lorenz/Git/emsdk/usptream/emscripten/src
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICLIB
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Dispd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Disp
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \
    Utils/datamarker.cpp \
    Utils/rawsettings.cpp \
    Utils/filteroperator.cpp \
    Utils/filterplotscene.cpp \
    Utils/butterflyscene.cpp \
    Utils/butterflysceneitem.cpp \
    Models/averagemodel.cpp \
    Models/rawmodel.cpp \
    Models/eventmodel.cpp \
    Delegates/averagedelegate.cpp \
    Delegates/rawdelegate.cpp \
    Delegates/eventdelegate.cpp \
    Windows/mainwindow.cpp \
    Windows/filterwindow.cpp \
    Windows/eventwindow.cpp \
    Windows/datawindow.cpp \
    Windows/aboutwindow.cpp \
    Windows/informationwindow.cpp \
    Windows/averagewindow.cpp \
    Windows/scalewindow.cpp \
    Windows/chinfowindow.cpp \
    Utils/datapackage.cpp \    
    Windows/noisereductionwindow.cpp

HEADERS += \
    Utils/datamarker.h \
    Utils/rawsettings.h \
    Utils/filteroperator.h \
    Utils/types.h \
    Utils/info.h \
    Utils/filterplotscene.h \
    Utils/butterflyscene.h \
    Utils/butterflysceneitem.h \
    Models/averagemodel.h \
    Models/rawmodel.h \
    Models/eventmodel.h \
    Delegates/averagedelegate.h \
    Delegates/rawdelegate.h \
    Delegates/eventdelegate.h \
    Windows/mainwindow.h \
    Windows/filterwindow.h \
    Windows/eventwindow.h \
    Windows/datawindow.h \
    Windows/aboutwindow.h \
    Windows/informationwindow.h \
    Windows/averagewindow.h \
    Windows/scalewindow.h \
    Windows/chinfowindow.h \
    Windows/noisereductionwindow.h \
    Utils/datapackage.h \

FORMS += \
    Windows/eventwindowdock.ui \
    Windows/datawindowdock.ui \
    Windows/mainwindow.ui \
    Windows/aboutwindow.ui \
    Windows/informationwindow.ui \
    Windows/averagewindow.ui \
    Windows/scalewindow.ui \
    Windows/chinfowindow.ui \
    Windows/filterwindowdock.ui \
    Windows/noisereductionwindow.ui

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

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
    mne_browse.qrc

# Icon
win32 {
    RC_FILE = Resources/Images/ApplicationIcons/mne_browse.rc
}
macx {
    ICON = Resources/Images/ApplicationIcons/mne_browse.icns
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

    # Copy resources to app bundle
    sgrc.path = Contents/MacOS/resources/general/
    sgrc.files = $${ROOT_DIR}/resources/general/selectionGroups
    QMAKE_BUNDLE_DATA += sgrc

    loutrc.path = Contents/MacOS/resources/general/
    loutrc.files = $${ROOT_DIR}/resources/general/2DLayouts

    filtrc.path = Contents/MacOS/resources/general/
    filtrc.files = $${ROOT_DIR}/resources/general/default_filters
    QMAKE_BUNDLE_DATA += filtrc

    hpirc.path = Contents/MacOS/resources/general/
    hpirc.files = $${ROOT_DIR}/resources/general/hpiAlignment
    QMAKE_BUNDLE_DATA += hpirc

    ssrc.path = Contents/MacOS/resources/general/
    ssrc.files = $${ROOT_DIR}/resources/general/sensorSurfaces
    QMAKE_BUNDLE_DATA += ssrc

    lout3rc.path = Contents/MacOS/resources/general/
    lout3rc.files = $${ROOT_DIR}/resources/general/3DLayouts
    QMAKE_BUNDLE_DATA += lout3rc
    QMAKE_BUNDLE_DATA += loutrc
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
