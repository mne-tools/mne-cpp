#--------------------------------------------------------------------------------------------------------------
#
# @file     fiff.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
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
# @brief    This project file builds the fiff library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT += network
QT -= gui

DEFINES += FIFF_LIBRARY

TARGET = Fiff
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}Utilsd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}Utils
}

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, build_MNECPP_Static_Lib) {
    CONFIG += staticlib
    DEFINES += BUILD_MNECPP_STATIC_LIB
}
else {
    CONFIG += dll

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
}

SOURCES += fiff.cpp \
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
    c/fiff_sparse_matrix.cpp

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
    c/fiff_types_mne-c.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/fiff

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# Deploy Qt Dependencies
win32 {
    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .dll
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    DEPLOY_COMMAND = windeployqt

    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    #  # Uncomment the following line to help debug the deploy command when running qmake
    #  warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}
