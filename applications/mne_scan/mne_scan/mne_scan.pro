#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_scan.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     February, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
#--------------------------------------------------------------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = app

QT += network core widgets xml

qtHaveModule(3dextras) {
    QT += 3dextras
}

TARGET = mne_scan

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

CONFIG += console #DEBUG

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Realtimed \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Realtime \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}Disp3D \
            -lscMeas \
            -lscDisp \
            -lscShared
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \
    startupwidget.cpp \
    runwidget.cpp \
    mainsplashscreen.cpp \
    pluginscene.cpp \
    pluginitem.cpp \
    plugingui.cpp \
    arrow.cpp \
    mainwindow.cpp

HEADERS += \
    info.h \
    startupwidget.h \
    runwidget.h \
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

    # Copy Resource folder to app bundle
    filtrc.path = Contents/MacOS/resources/general/default_filters
    filtrc.files = $${ROOT_DIR}/resources/general/default_filters
    QMAKE_BUNDLE_DATA += filtrc

    sgrc.path = Contents/MacOS/resources/general/selectionGroups
    sgrc.files = $${ROOT_DIR}/resources/general/selectionGroups
    QMAKE_BUNDLE_DATA += sgrc

    loutrc.path = Contents/MacOS/resources/general/2DLayouts
    loutrc.files = $${ROOT_DIR}/resources/general/2DLayouts
    QMAKE_BUNDLE_DATA += loutrc

    hpirc.path = Contents/MacOS/resources/general/hpiAlignment
    hpirc.files = $${ROOT_DIR}/resources/general/hpiAlignment
    QMAKE_BUNDLE_DATA += hpirc

    ssrc.path = Contents/MacOS/resources/general/sensorSurfaces
    ssrc.files = $${ROOT_DIR}/resources/general/sensorSurfaces
    QMAKE_BUNDLE_DATA += ssrc

    lout3rc.path = Contents/MacOS/resources/general/3DLayouts
    lout3rc.files = $${ROOT_DIR}/resources/general/3DLayouts
    QMAKE_BUNDLE_DATA += lout3rc

    rcplugins.path = Contents/MacOS/resources/mne_scan/
    rcplugins.files = $${ROOT_DIR}/resources/mne_scan/plugins
    QMAKE_BUNDLE_DATA += rcplugins

    plugins.path = Contents/MacOS/
    plugins.files = $${ROOT_DIR}/bin/mne_scan_plugins
    QMAKE_BUNDLE_DATA += plugins

    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .app
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    DEPLOY_COMMAND = macdeployqt
    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
    # Set arg(s) to libpath to find all libs needed to copy into app
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -libpath=$${MNE_LIBRARY_DIR}
    QMAKE_CLEAN += -r $${DEPLOY_TARGET}

}
