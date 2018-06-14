
#--------------------------------------------------------------------------------------------------------------
#
# @file     utils.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
# @brief    This project file builds the Utils library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT -= gui
QT += xml core
QT += concurrent # Check with HP-UX

DEFINES += UTILS_LIBRARY

TARGET = Utils
TARGET = $$join(TARGET,,MNE$$MNE_LIB_VERSION,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
}
else {
    CONFIG += dll
}

SOURCES += \
    kmeans.cpp \
    mnemath.cpp \
    ioutils.cpp \
    layoutloader.cpp \
    layoutmaker.cpp \
    mp/adaptivemp.cpp \
    mp/atom.cpp \
    mp/fixdictmp.cpp \
    selectionio.cpp \
    filterTools/cosinefilter.cpp \
    filterTools/parksmcclellan.cpp \
    filterTools/filterdata.cpp \
    filterTools/filterio.cpp \
    detecttrigger.cpp \
    spectrogram.cpp \
    warp.cpp \
    filterTools/sphara.cpp \
    sphere.cpp \
    generics/buffer.cpp \
    generics/circularbuffer.cpp \
    generics/circularmatrixbuffer.cpp \
    generics/observerpattern.cpp \
    spectral.cpp


HEADERS += \
    kmeans.h\
    utils_global.h \
    mnemath.h \
    ioutils.h \
    layoutloader.h \
    layoutmaker.h \
    mp/adaptivemp.h \
    mp/atom.h \
    mp/fixdictmp.h \
    selectionio.h \
    layoutmaker.h \
    filterTools/cosinefilter.h \
    filterTools/parksmcclellan.h \
    filterTools/filterdata.h \
    filterTools/filterio.h \
    detecttrigger.h \
    spectrogram.h \
    warp.h \
    filterTools/sphara.h \
    sphere.h \
    simplex_algorithm.h \
    generics/buffer.h \
    generics/circularbuffer.h \
    generics/circularbuffer_old.h \
    generics/circularmatrixbuffer.h \
    generics/circularmultichannelbuffer_old.h \
    generics/commandpattern.h \
    generics/observerpattern.h \
    generics/typename_old.h \
    spectral.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/utils

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# Deploy library
win32 {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
