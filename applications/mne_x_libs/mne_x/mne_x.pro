#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_x.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     March, 2013
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
#     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds the mne_x library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = lib

QT += widgets

DEFINES += MNE_X_LIBRARY

TARGET = mne_x
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lxMeasd \
            -lxDispd \
            -lxDtMngd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lxMeas \
            -lxDisp \
            -lxDtMng
}

DESTDIR = $${MNE_LIBRARY_DIR}

#
# win32: copy dll's to bin dir
# unix: add lib folder to LD_LIBRARY_PATH
#
win32 {
    FILE = $${DESTDIR}/$${TARGET}.dll
    BINDIR = $${DESTDIR}/../bin
    FILE ~= s,/,\\,g
    BINDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${BINDIR}) $$escape_expand(\\n\\t)
}

SOURCES += \
    Management/connector.cpp \
    Management/pluginmanager.cpp \
    Management/pluginconnector.cpp \
    Management/plugininputconnector.cpp \
    Management/pluginoutputconnector.cpp \
    Management/plugininputdata.cpp \
    Management/pluginoutputdata.cpp \
    Management/pluginset.cpp \
    Management/pluginconnectorconnection.cpp \
    Management/pluginstage.cpp


HEADERS += \
    mne_x_global.h \
    Interfaces/ISensor.h \
    Interfaces/IPlugin.h \
    Management/connector.h \
    Management/pluginmanager.h \
    Management/pluginconnector.h \
    Management/plugininputconnector.h \
    Management/pluginoutputconnector.h \
    Management/plugininputdata.h \
    Management/pluginoutputdata.h \
    Management/pluginset.h \
    Management/pluginconnectorconnection.h \
    Interfaces/IIO.h \
    Interfaces/IAlgorithm.h \
    Management/pluginstage.h


INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_X_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_X_INCLUDE_DIR}/mne_x

header_files_interfaces.files = ./Interfaces/*.h
header_files_interfaces.path = $${MNE_X_INCLUDE_DIR}/mne_x/Interfaces

INSTALLS += header_files
INSTALLS += header_files_interfaces

