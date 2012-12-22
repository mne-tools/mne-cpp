#--------------------------------------------------------------------------------------------------------------
#
# @file     mne.pro
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
# @brief    This project file builds the mne library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT += network
QT -= gui

DEFINES += MNE_LIBRARY

TARGET = mne

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${PWD}/../../lib/
CONFIG(debug, debug|release) {
    LIBS += -lfiffd \
            -lfsd \
            -lmne_mathd
}
else {
    LIBS += -lfiff \
            -lfs \
            -lmne_math
}


DESTDIR = $${PWD}/../../lib

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

SOURCES +=  mne.cpp \
            mne_sourcespace.cpp \
            mne_forwardsolution.cpp \
            mne_hemisphere.cpp \
            mne_inverse_operator.cpp \
            mne_epoch_data.cpp \
            mne_epoch_data_list.cpp \
            mne_math.cpp \
            mne_rt_cmd_client.cpp \
            mne_rt_data_client.cpp \
            mne_rt_client.cpp

HEADERS +=  mne.h \
            mne_global.h \
#            hpcmatrix.h \
            mne_sourcespace.h \
            mne_hemisphere.h \
            mne_forwardsolution.h \
            mne_inverse_operator.h \
            mne_epoch_data.h \
            mne_epoch_data_list.h \
            mne_math.h \
            mne_rt_cmd_client.h \
            mne_rt_data_client.h \
            mne_rt_client.h

INCLUDEPATH += $${PWD}/../../include/3rdParty

#Install headers to include directory
header_files.files = ./*.h
header_files.path = ../../include/mne

INSTALLS += header_files
