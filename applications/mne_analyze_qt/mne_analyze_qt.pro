#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_analyze_qt.pro
# @author   Franco Polo <Franco-Joel.Polo@tu-ilmenau.de>;
#			Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
# @version  1.0
# @date     January, 2015
#
# @section  LICENSE
#
# Copyright (C) 2015, Franco Polo, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
# @brief    This project file builds the mne_analyze_qt project
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT += gui widgets 3dextras

TARGET = mne_analyze_qt

#If one single executable is to be build
#-> comment out flag in .pri file
#-> add DEFINES += BUILD_MNECPP_STATIC_LIB in projects .pro file
#-> This needs to be done in order to avoid problem with the Q_DECL_EXPORT/Q_DECL_IMPORT flag in the global headers
contains(MNECPP_CONFIG, build_MNECPP_Static_Lib) {
    DEFINES += BUILD_MNECPP_STATIC_LIB
}

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

#Note that the static flag is ingored when building against a dynamic qt version
CONFIG += static console #DEBUG console

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}Disp3D
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \   
    Windows/mainwindow.cpp \
    Windows/aboutwindow.cpp \
    Windows/viewerwidget.cpp \
    Views/baseview.cpp \
    Views/view3danalyze.cpp

HEADERS += \
    info.h \
    Windows/mainwindow.h \
    Windows/aboutwindow.h \
    Windows/viewerwidget.h \
    Views/baseview.h \
    Views/view3danalyze.h

FORMS += \
    Windows/mainwindow.ui \
    Windows/aboutwindow.ui \
    Windows/viewerwidget.ui

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_X_INCLUDE_DIR}

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
    mneanalyzeqt.qrc

# Icon
win32 {
    RC_FILE = resources/images/appIcons/mne-analyze.rc
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

#    isEmpty(TARGET_EXT) {
#        TARGET_CUSTOM_EXT = .app
#    } else {
#        TARGET_CUSTOM_EXT = $${TARGET_EXT}
#    }

#    # Copy libs
#    BUNDLEFRAMEDIR = $$shell_quote($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}/Contents/Frameworks)
#    QMAKE_POST_LINK = $${QMAKE_MKDIR} $${BUNDLEFRAMEDIR} &
#    QMAKE_POST_LINK += $${QMAKE_COPY} $${MNE_LIBRARY_DIR}/{libMNE1Generics.*,libMNE1Utils.*,libMNE1Fs.*,libMNE1Fiff.*,libMNE1Mne*,libMNE1Inverse.*,libMNE1Disp.*,libMNE1Disp3D.*} $${BUNDLEFRAMEDIR}

#    DEPLOY_COMMAND = macdeployqt
#    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
#    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -verbose=0
}
