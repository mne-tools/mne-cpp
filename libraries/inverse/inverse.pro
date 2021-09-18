#==============================================================================================================
#
# @file     inverse.pro
# @author   Lorenz Esch <lesch@mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
# @brief    This project file builds the inverse library.
#
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT -= gui
QT += concurrent

DEFINES += INVERSE_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Inverse
TARGET = $$join(TARGET,,mnecpp,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppFwdd \
            -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppFwd \
            -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    inverse_global.cpp \
    minimumNorm/minimumnorm.cpp \
    rapMusic/rapmusic.cpp \
    rapMusic/pwlrapmusic.cpp \
    rapMusic/dipole.cpp \
    dipoleFit/dipole_fit.cpp \
    dipoleFit/dipole_fit_data.cpp \
    dipoleFit/dipole_fit_settings.cpp \
    dipoleFit/dipole_forward.cpp \
    dipoleFit/ecd.cpp \
    dipoleFit/ecd_set.cpp \
    dipoleFit/guess_data.cpp \
    c/mne_inverse_operator.cpp \
    c/mne_meas_data.cpp \
    c/mne_meas_data_set.cpp \
    hpiFit/hpifit.cpp \
    hpiFit/hpifitdata.cpp

HEADERS +=\
    inverse_global.h \
    IInverseAlgorithm.h \
    minimumNorm/minimumnorm.h \
    rapMusic/rapmusic.h \
    rapMusic/pwlrapmusic.h \
    rapMusic/dipole.h \
    dipoleFit/analyze_types.h \
    dipoleFit/dipole_fit.h \
    dipoleFit/dipole_fit_data.h \
    dipoleFit/dipole_fit_settings.h \
    dipoleFit/dipole_forward.h \
    dipoleFit/ecd.h \
    dipoleFit/ecd_set.h \
    dipoleFit/guess_data.h \
    c/mne_inverse_operator.h \
    c/mne_meas_data.h \
    c/mne_meas_data_set.h \
    hpiFit/hpifit.h \
    hpiFit/hpifitdata.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    # OpenMP
    !contains(MNECPP_CONFIG, wasm) {
        QMAKE_CXXFLAGS  +=  -openmp
        #QMAKE_LFLAGS    +=  -openmp
    }

    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
}

unix:!macx:!contains(MNECPP_CONFIG, wasm):!contains(MNECPP_CONFIG, static) {
    QMAKE_CXXFLAGS  +=  -fopenmp
    QMAKE_LFLAGS    +=  -fopenmp
}

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
}

# Activate FFTW backend in Eigen for non-static builds only
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

################################################## BUILD TIMESTAMP/HASH UPDATER ############################################

FILETOUPDATE = inverse_global.cpp

ALLFILES += $$HEADERS
ALLFILES += $$SOURCES
ALLFILES -= $$FILETOUPDATE
FileUpdater.target = phonyFileUpdater
unix|macx {
    FileUpdater.commands = touch $$PWD/$$FILETOUPDATE ; echo PASTA > phonyFileUpdater
}
win32 {
    FileUpdater.commands = copy $$PWD/$$FILETOUPDATE +,, & echo PASTA > phonyFileUpdater
}
for (IFILE, ALLFILES) {
    FileUpdater.depends += $$PWD/$$IFILE
}
PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater

