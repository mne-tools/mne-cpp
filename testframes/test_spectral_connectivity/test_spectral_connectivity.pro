#--------------------------------------------------------------------------------------------------------------
#
# @file     test_spectral_connectivity.pro
# @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     May, 2018
#
# @section  LICENSE
#
# Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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
# @brief    Builds the spectral connectivity unit test
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

VERSION = $${MNE_CPP_VERSION}

QT += testlib
QT -= gui

CONFIG   += console
CONFIG   -= app_bundle

TARGET = test_spectral_connectivity

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Connectivityd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Connectivity
}

DESTDIR =  $${MNE_BINARY_DIR}

SOURCES += \
    test_spectral_connectivity.cpp

HEADERS += \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    LIBS += -lgcov
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
}
