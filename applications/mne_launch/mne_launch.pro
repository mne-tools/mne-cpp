#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_launch.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     November, 2016
#
# @section  LICENSE
#
# Copyright (C) 2016, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
# @brief    This project file builds the MNE Launch application.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

QT += qml quick

TARGET = mne_launch

CONFIG += c++11

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += main.cpp \
    application.cpp \
    mnelaunchcontrol.cpp

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_SCAN_INCLUDE_DIR}

RESOURCES +=    qml.qrc \
                images.qrc


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
    # remember to also deploy qml depnedencies with --qmldir $${PWD} $${DESTDIR}
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} --qmldir $${PWD}/qml $${DESTDIR}
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

    DEPLOY_COMMAND = macdeployqt
    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
    # Set arg(s) with libpath and qmldir for Quick frameworks
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -libpath=$${MNE_LIBRARY_DIR} -qmldir=$${PWD}/qml
    QMAKE_CLEAN += -r $${DEPLOY_TARGET}

    # fix up app bundle via install target to have soft links to enable running external binaries
    browse.path = $${DEPLOY_TARGET}/Contents/MacOS/
    browse.extra = (cd $${DEPLOY_TARGET}/Contents/MacOS && ln -s ../../../mne_browse.app/Contents/MacOS/mne_browse .)
    analyze.path = $${DEPLOY_TARGET}/Contents/MacOS/
    analyze.extra = (cd $${DEPLOY_TARGET}/Contents/MacOS && ln -s ../../../mne_analyze.app/Contents/MacOS/mne_analyze .)
    scan.path = $${DEPLOY_TARGET}/Contents/MacOS/
    scan.extra = (cd $${DEPLOY_TARGET}/Contents/MacOS && ln -s ../../../mne_scan.app/Contents/MacOS/mne_scan .)
    INSTALLS += browse analyze scan
}

HEADERS += \
    application.h \
    mnelaunchcontrol.h
