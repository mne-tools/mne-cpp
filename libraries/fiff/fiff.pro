#==============================================================================================================
#
# @file     fiff.pro
# @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
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
# @brief    This project file builds the fiff library.
#
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += network
QT -= gui

DESTDIR = $${MNE_LIBRARY_DIR}

DEFINES += FIFF_LIBRARY

TARGET = Fiff
TARGET = $$join(TARGET,,mnecpp,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, wasm) {
    DEFINES += WASMBUILD
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppUtilsd \
} else {
    LIBS += -lmnecppUtils \
}

SOURCES += fiff.cpp \
    fiff_global.cpp \
    fiff_tag.cpp \
    fiff_coord_trans.cpp \
    fiff_ch_info.cpp \
    fiff_proj.cpp \
    fiff_named_matrix.cpp \
    fiff_raw_data.cpp \
    fiff_ctf_comp.cpp \
    fiff_id.cpp \
    fiff_info.cpp \
    fiff_raw_dir.cpp \
    fiff_dig_point.cpp \
    fiff_ch_pos.cpp \
    fiff_cov.cpp \
    fiff_stream.cpp \
    fiff_dir_entry.cpp \
    fiff_info_base.cpp \
    fiff_evoked.cpp \
    fiff_evoked_set.cpp \
    fiff_io.cpp \
    fiff_dig_point_set.cpp \
    fiff_dir_node.cpp \
    c/fiff_coord_trans_old.cpp \
    c/fiff_sparse_matrix.cpp \
    c/fiff_digitizer_data.cpp \
    c/fiff_coord_trans_set.cpp \
    fifffilesharer.cpp

HEADERS += fiff.h \
    fiff_global.h \
    fiff_explain.h \
    fiff_file.h \
    fiff_types.h \
    fiff_id.h \
    fiff_constants.h \
    fiff_tag.h \
    fiff_coord_trans.h \
    fiff_ch_info.h \
    fiff_proj.h \
    fiff_named_matrix.h \
    fiff_ctf_comp.h \
    fiff_info.h \
    fiff_raw_data.h \
    fiff_dir_entry.h \
    fiff_raw_dir.h \
    fiff_dig_point.h \
    fiff_ch_pos.h \
    fiff_cov.h \
    fiff_stream.h \
    fiff_info_base.h \
    fiff_evoked.h \
    fiff_evoked_set.h \
    fiff_io.h \
    fiff_dig_point_set.h \
    fiff_dir_node.h \
    c/fiff_coord_trans_old.h \
    c/fiff_sparse_matrix.h \
    c/fiff_types_mne-c.h \
    c/fiff_digitizer_data.h \
    c/fiff_coord_trans_set.h \
    fifffilesharer.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
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

FILE_TO_UPDATE = fiff_global.cpp
win32 {
    CONFIG(debug, debug|release) {
        OBJ_TARJET = debug\fiff_global.obj
    } else {
        OBJ_TARJET = release\fiff_global.obj
    }
}

ALL_FILES += $$HEADERS
ALL_FILES += $$SOURCES
ALL_FILES -= $$FILE_TO_UPDATE

FileUpdater.target = phonyFileUpdater
for (I_FILE, ALL_FILES) {
    FileUpdater.depends += $${PWD}/$${I_FILE}
}

unix|macx {
    FileUpdater.commands = touch $${PWD}/$${FILE_TO_UPDATE} ; echo PASTA > phonyFileUpdater
}

win32 {
    FileUpdater.commands = copy /y $$shell_path($${PWD})\$${FILE_TO_UPDATE} +,, $$shell_path($${PWD})\$${FILE_TO_UPDATE} & echo PASTA > phonyFileUpdater
    OrderForcerTarget.target = $${OBJ_TARJET}
    OrderForcerTarget.depends += phonyFileUpdater
    QMAKE_EXTRA_TARGETS += OrderForcerTarget
}

PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater
