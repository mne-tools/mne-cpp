#--------------------------------------------------------------------------------------------------------------
#
# @file     examples.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
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
# @brief    This project file builds all examples of the mne-cpp project.
#
#--------------------------------------------------------------------------------------------------------------

include(../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    ex_cancel_noise \
    ex_compute_inverse \
    ex_compute_inverse_raw \
    ex_evoked_grad_amp \
    ex_fiff_io \
    ex_find_evoked \
    ex_make_inverse_operator \
    ex_make_layout \
    ex_read_bem \
    ex_read_epochs \
    ex_read_evoked \
    ex_read_fwd \
    ex_read_raw \
    ex_read_write_raw

!contains(MNECPP_CONFIG, minimalVersion) {
    qtHaveModule(charts) {
        SUBDIRS += \
            ex_clustered_inverse \
            ex_compute_inverse_pwl_rap_music \
            ex_compute_inverse_rap_music \
            ex_connectivity_tutorial \
            ex_disp \
            ex_disp_3D_tutorial \
            ex_fs_surface \
            ex_histogram \
            ex_read_fwd_disp_3D \
            ex_raw_clustered_inverse \
            ex_raw_clustered_inverse_pwl_rap_music \
            ex_raw_clustered_inverse_rap_music \
            ex_roi_clustered_inverse_pwl_rap_music \
            ex_st_clustered_inverse_pwl_rap_music
    }
    else {
        message("examples.pro - The Qt Charts module is missing. Please install to build the complete set of MNE-CPP features.")
    }
}
