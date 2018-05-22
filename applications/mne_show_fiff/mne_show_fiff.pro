#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_show_fiff.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     November, 2016
#
# @section  LICENSE
#
# Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
# @brief    Implements mne_show_fiff application of MNE-C
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT -= gui

VERSION = $${MNE_CPP_VERSION}

CONFIG   += console
CONFIG   -= app_bundle

TARGET = mne_show_fiff

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne
}

DESTDIR =  $${MNE_BINARY_DIR}

SOURCES += \
    main.cpp \
    mne_fiff_exp.cpp \
    mne_fiff_exp_set.cpp \
    mne_show_fiff_settings.cpp

HEADERS += \
    mne_fiff_exp.h \
    mne_fiff_exp_set.h \
    mne_show_fiff_settings.h

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/general/explanations/fiff_explanations.txt \

# Copy resource files to bin resource folder
for(FILE, RESOURCE_FILES) {
    FILEDIR = $$dirname(FILE)
    FILEDIR ~= s,/resources,/bin/resources,g
    FILEDIR = $$shell_path($${FILEDIR})
    TRGTDIR = $${FILEDIR}

    QMAKE_POST_LINK += $$sprintf($${QMAKE_MKDIR_CMD}, "$${TRGTDIR}") $$escape_expand(\n\t)

    FILE = $$shell_path($${FILE})
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${TRGTDIR}) $$escape_expand(\\n\\t)
}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

macx {

    # Mac now creates app bundle
    CONFIG += app_bundle

    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .app
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    QMAKE_RPATHDIR += @executable_path/../Frameworks

    # Copy Resource folder to app bundle

    explain.path = Contents/MacOS/resources/general/explanations
    explain.files = $${ROOT_DIR}/resources/general/explanations/fiff_explanations.txt
    QMAKE_BUNDLE_DATA += explain

    DEPLOY_COMMAND = macdeployqt
    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
    # Set arg(s) to libpath to find all libs needed to copy into app
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} -libpath=$${MNE_LIBRARY_DIR}
    QMAKE_CLEAN += -r $${DEPLOY_TARGET}

}
