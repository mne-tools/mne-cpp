#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_plugin_creator.pro
# @author   Erik Hornberger <erik.hornberger@shi-g.com>;
# @version  1.0
# @date     October, 2018
#
# @section  LICENSE
#
# Copyright (C) 2018, Sumitomo Heavy Industries, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
# @brief    build and distribution settings for mne_plugin_creator
#
#--------------------------------------------------------------------------------------------------------------
include(../../mne-cpp.pri)

QT -= gui

VERSION = $${MNE_CPP_VERSION}

# Target (executable) name
TARGET = mne_plugin_creator
contains(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

# Configuration options
CONFIG += c++11 console
CONFIG -= app_bundle
contains(MNECPP_CONFIG, static) {
    CONFIG += static
}

# Files
SOURCES += \
    main.cpp \
    pluginparams.cpp \
    templatefile.cpp \
    mnescanplugincreator.cpp \
    iplugincreator.cpp \
    mnescaninputparser.cpp \
    iinputparser.cpp \
    appinputparser.cpp

HEADERS += \
    pluginparams.h \
    templatefile.h \
    mnescanplugincreator.h \
    iplugincreator.h \
    mnescaninputparser.h \
    iinputparser.h \
    appinputparser.h

RESOURCE_FILES += \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/README.md \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/options.png \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/template.json \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/template.pro \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/template_global.h \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/headertemplate.h \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/sourcetemplate.cpp \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/widgettemplate.h \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/widgettemplate.cpp \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/widgettemplate.ui \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/aboutwidgettemplate.h \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/aboutwidgettemplate.cpp \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/aboutwidgettemplate.ui \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/setupwidgettemplate.h \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/setupwidgettemplate.cpp \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/mne_scan/setupwidgettemplate.ui \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/testframes/template.pro \
    $${ROOT_DIR}/resources/mne_plugin_creator/templates/testframes/template.cpp \


OTHER_FILES += \
    README.md

# Deployment
unix: QMAKE_CXXFLAGS += -isystem

win32 {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployAppArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${LIBS},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}

macx {
    QMAKE_RPATHDIR += @executable_path/../Frameworks

    # Copy Resource folder to app bundle
    explain.path = Contents/MacOS/resources/general/explanations
    explain.files = $${ROOT_DIR}/resources/general/explanations/fiff_explanations.txt
    QMAKE_BUNDLE_DATA += explain
    EXTRA_ARGS =

    # Entries returned in DEPLOY_CMD
    DEPLOY_CMD = $$macDeployArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}

    QMAKE_CLEAN += -r $$member(DEPLOY_CMD, 1)
}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

# Copy resources to install location
COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}
message(COPYING RESOURCES: $${COPY_CMD})

