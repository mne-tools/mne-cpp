#--------------------------------------------------------------------------------------------------------------
#
# @file     app.pro
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
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}DispChartsd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
            -lscMeasd \
            -lscDispd \
            -lscSharedd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}DispCharts \
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
    mainwindow.cpp \
    mainsplashscreen.cpp \
    pluginscene.cpp \
    pluginitem.cpp \
    plugingui.cpp \
    arrow.cpp

HEADERS += \
    info.h \
    startupwidget.h \
    runwidget.h \
    mainwindow.h \
    mainsplashscreen.h \
    pluginscene.h \
    pluginitem.h \
    plugingui.h \
    arrow.h

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
    mne_scan_rc.path = Contents/MacOS
    mne_scan_rc.files = $${DESTDIR}/mne_scan_libs
    QMAKE_BUNDLE_DATA += mne_scan_rc

    plugins.path = Contents/MacOS
    plugins.files = $${DESTDIR}/mne_scan_plugins
    QMAKE_BUNDLE_DATA += plugins

#    isEmpty(TARGET_EXT) {
#        TARGET_CUSTOM_EXT = .app
#    } else {
#        TARGET_CUSTOM_EXT = $${TARGET_EXT}
#    }

#    # Copy libs
#    BUNDLEFRAMEDIR = $$shell_quote($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}/Contents/Frameworks)
#    QMAKE_POST_LINK = $${QMAKE_MKDIR} $${BUNDLEFRAMEDIR} &
#    QMAKE_POST_LINK += $${QMAKE_COPY} $${MNE_LIBRARY_DIR}/{libMNE1Generics.*,libMNE1Utils.*,libMNE1Fs.*,libMNE1Fiff.*,libMNE1Mne*,libMNE1Disp.*} $${BUNDLEFRAMEDIR}

#    DEPLOY_COMMAND = macdeployqt
#    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
#    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -verbose=0
}
