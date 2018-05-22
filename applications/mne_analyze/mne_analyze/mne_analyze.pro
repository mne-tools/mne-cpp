#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_analyze.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     January, 2017
#
# @section  LICENSE
#
# Copyright (C) 2017, hristoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

QT += gui widgets 3dextras
qtHaveModule(printsupport): QT += printsupport

TARGET = mne_analyze

#If one single executable is to be build
#-> comment out flag in .pri file
#-> add DEFINES += BUILD_STATIC_LIBRARIES in projects .pro file
#-> This needs to be done in order to avoid problem with the Q_DECL_EXPORT/Q_DECL_IMPORT flag in the global headers
contains(MNECPP_CONFIG, buildStaticLibraries) {
    DEFINES += BUILD_STATIC_LIBRARIES
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

#Note that the static flag is ingored when building against a dynamic qt version
CONFIG += static console #DEBUG console

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
            -lanSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}Disp3D \
            -lanShared
}


!isEmpty( CNTK_INCLUDE_DIR ) {
    LIBS += -L$${CNTK_LIBRARY_DIR}
    CONFIG(debug, debug|release) {
        LIBS += -lMNE$${MNE_LIB_VERSION}Deepd \
                -lCntk.Core-2.0 \

    }
    else {
        LIBS += -lMNE$${MNE_LIB_VERSION}Deep \
                -lCntk.Core-2.0 \
    }
}


DESTDIR = $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mdiview.cpp

HEADERS += \
    info.h \
    mainwindow.h \
    mdiview.h

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

# Deploy Qt Dependencies
win32 {
    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .exe
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    DEPLOY_COMMAND = windeployqt

    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    #  # Uncomment the following line to help debug the deploy command when running qmake
    #  warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}
unix:!macx {
    # === Unix ===
    QMAKE_RPATHDIR += $ORIGIN/../lib
}
macx {
    # === Mac ===
    QMAKE_RPATHDIR += @executable_path/../Frameworks

    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .app
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    extensions.path = Contents/MacOS/
    extensions.files = $${ROOT_DIR}/applications/mne_analyze/extensions
    QMAKE_BUNDLE_DATA += extensions

    DEPLOY_COMMAND = macdeployqt
    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
    # Set arg(s) to libpath to find all libs needed to copy into app
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -libpath=$${MNE_LIBRARY_DIR}
    QMAKE_CLEAN += -r $${DEPLOY_TARGET}
}
