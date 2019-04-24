#--------------------------------------------------------------------------------------------------------------
#
# @file     ex_edf_to_fif.pro
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
# @brief    Builds an example on how to use the new EDF utilities.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

VERSION = $${MNE_CPP_VERSION}

QT += gui widgets charts

CONFIG   += console
CONFIG   -= app_bundle

TARGET = ex_edf_to_fif

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd \
            -lMNE$${MNE_LIB_VERSION}Inversed \
            -lMNE$${MNE_LIB_VERSION}Connectivityd \
            -lMNE$${MNE_LIB_VERSION}RtProcessingd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}Disp3Dd \
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd \
            -lMNE$${MNE_LIB_VERSION}Inverse \
            -lMNE$${MNE_LIB_VERSION}Connectivity \
            -lMNE$${MNE_LIB_VERSION}RtProcessing \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}Disp3D \
}

DESTDIR =  $${MNE_BINARY_DIR}

SOURCES +=  main.cpp \
            edfinfo.cpp \

HEADERS +=  edfinfo.h \

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

win32 {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployAppArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${LIBS},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}
unix:!macx {
    # === Unix ===
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

# Activate FFTW backend in Eigen
contains(MNECPP_CONFIG, useFFTW) {
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
