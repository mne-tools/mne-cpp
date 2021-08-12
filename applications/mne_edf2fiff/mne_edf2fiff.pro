#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_edf2fiff.pro
# @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     April, 2019
#
# @section  LICENSE
#
# Copyright (C) 2019, Simon Heinke and Matti Hamalainen. All rights reserved.
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
# @brief    Builds an executable that converts raw EDF into raw Fiff
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT += network concurrent

CONFIG += console

DESTDIR =  $${MNE_BINARY_DIR}

TARGET = mne_edf2fiff
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

!contains(MNECPP_CONFIG, withAppBundles) {
    CONFIG -= app_bundle
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICBUILD
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppFiffd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppFiff \
            -lmnecppUtils \
}

SOURCES +=  main.cpp \
            edf_info.cpp \
            edf_raw_data.cpp \
            edf_ch_info.cpp \

HEADERS +=  edf_info.h \
            edf_raw_data.h \
            edf_ch_info.h \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_GLOBAL_DIR}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

macx {
    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/../lib
}

# Activate FFTW backend in Eigen
contains(MNECPP_CONFIG, useFFTW):!contains(MNECPP_CONFIG, static) {
    DEFINES += EIGEN_FFTW_DEFAULT
    INCLUDEPATH += $$shell_path($${FFTW_DIR_INCLUDE})
    LIBS += -L$$shell_path($${FFTW_DIR_LIBS})

    win32 {
        # On Windows
        LIBS += -llibfftw3-3 \
                -llibfftw3f-3 \
                -llibfftw3l-3 \
    }

    unix:!macx {
        # On Linux
        LIBS += -lfftw3 \
                -lfftw3_threads \
    }
}
