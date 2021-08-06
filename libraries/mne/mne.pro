#==============================================================================================================
#
# @file     mne.pro
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
# @brief    This project file builds the mne library.
#
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += network concurrent
QT -= gui

DEFINES += MNE_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Mne
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
    LIBS += -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
}

SOURCES += \
    mne.cpp \
    mne_sourcespace.cpp \
    mne_forwardsolution.cpp \
    mne_sourceestimate.cpp \
    mne_hemisphere.cpp \
    mne_inverse_operator.cpp \
    mne_epoch_data.cpp \
    mne_epoch_data_list.cpp \
    mne_cluster_info.cpp \
    mne_surface.cpp \
    mne_corsourceestimate.cpp\
    mne_bem.cpp\
    mne_bem_surface.cpp \
    mne_project_to_surface.cpp \
    c/mne_cov_matrix.cpp \
    c/mne_ctf_comp_data.cpp \
    c/mne_ctf_comp_data_set.cpp \
    c/mne_deriv.cpp \
    c/mne_deriv_set.cpp \
    c/mne_mne_data.cpp \
    c/mne_named_matrix.cpp \
    c/mne_proj_item.cpp \
    c/mne_proj_op.cpp \
    c/mne_raw_buf_def.cpp \
    c/mne_raw_data.cpp \
    c/mne_raw_info.cpp \
    c/mne_sss_data.cpp \
    c/mne_triangle.cpp \
    c/mne_vol_geom.cpp \
    c/mne_nearest.cpp \
    c/mne_patch_info.cpp \
    c/mne_source_space_old.cpp \
    c/mne_surface_old.cpp \
    c/mne_surface_or_volume.cpp \
    c/filter_thread_arg.cpp \
    c/mne_msh_display_surface.cpp \
    c/mne_msh_display_surface_set.cpp \
    c/mne_msh_picked.cpp \
    c/mne_morph_map.cpp \
    c/mne_msh_color_scale_def.cpp \
    c/mne_proj_data.cpp \
    c/mne_msh_light.cpp\
    c/mne_msh_light_set.cpp \
    c/mne_surface_patch.cpp \
    c/mne_msh_eyes.cpp \
    c/mne_mgh_tag.cpp \
    c/mne_mgh_tag_group.cpp

HEADERS += \
    mne.h \
    mne_global.h \
    mne_sourcespace.h \
    mne_hemisphere.h \
    mne_forwardsolution.h \
    mne_sourceestimate.h \
    mne_inverse_operator.h \
    mne_epoch_data.h \
    mne_epoch_data_list.h \
    mne_cluster_info.h \
    mne_surface.h \
    mne_corsourceestimate.h\
    mne_bem.h\
    mne_bem_surface.h \
    mne_project_to_surface.h \
    c/mne_cov_matrix.h \
    c/mne_ctf_comp_data.h \
    c/mne_ctf_comp_data_set.h \
    c/mne_deriv.h \
    c/mne_deriv_set.h \
    c/mne_mne_data.h \
    c/mne_named_matrix.h \
    c/mne_proj_item.h \
    c/mne_proj_op.h \
    c/mne_raw_buf_def.h \
    c/mne_raw_data.h \
    c/mne_raw_info.h \
    c/mne_sss_data.h \
    c/mne_triangle.h \
    c/mne_types.h \
    c/mne_types_mne-c.h \
    c/mne_vol_geom.h \
    c/mne_nearest.h \
    c/mne_patch_info.h \
    c/mne_source_space_old.h \
    c/mne_surface_old.h \
    c/mne_surface_or_volume.h \
    c/filter_thread_arg.h \
    c/mne_msh_display_surface.h \
    c/mne_msh_display_surface_set.h \
    c/mne_msh_picked.h \
    c/mne_morph_map.h \
    c/mne_msh_color_scale_def.h \
    c/mne_proj_data.h\
    c/mne_msh_light.h\
    c/mne_msh_light_set.h \
    c/mne_surface_patch.h \
    c/mne_msh_eyes.h \
    c/mne_mgh_tag.h \
    c/mne_mgh_tag_group.h

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
