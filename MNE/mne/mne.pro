#--------------------------------------------------------------------------------------------------------------
#
# @file     mne.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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

QT       += core
QT       -= gui

DEFINES += MNE_LIBRARY

TARGET = mne

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

win32:QMAKE_POST_LINK += $${QMAKE_COPY} "..\\..\\..\\mne-cpp\\lib\\$${TARGET}.dll" "..\\..\\..\\mne-cpp\\bin\\"
DESTDIR = $${PWD}/../../lib

CONFIG(debug, debug|release) {
    LIBS += -L$${PWD}/../../lib/ -lfiffd
}
else {
    LIBS += -L$${PWD}/../../lib -lfiff
}

SOURCES += mne.cpp \
    mne_sourcespace.cpp \
    mne_forwardsolution.cpp \
    mne_hemisphere.cpp \
    mne_epoch_data.cpp \
    mne_epoch_data_list.cpp \
    mne_inverse_operator.cpp \
    mne_cov.cpp

HEADERS += mne.h\
        mne_global.h \
#    hpcmatrix.h \
    mne_sourcespace.h \
    mne_hemisphere.h \
    mne_forwardsolution.h \
    mne_epoch_data.h \
    mne_epoch_data_list.h \
    mne_inverse_operator.h \
    mne_cov.h

#Install headers to include directory
header_files.files = ./*.h
header_files.path = ../../include/mne

INSTALLS += baseheader_files \
            header_files
