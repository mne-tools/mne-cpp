#==============================================================================================================
#
# @file     applications.pro
# @author   Chiran Doschi <Chiran.Doschi@childrens.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Florian Schlembach <Florian.Schlembach@tu-ilmenau.de>;
#           Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>;
#           Seok Lew <Seok.Lew@childrens.harvard.edu>;
#           Tim Kunze <Tim.Kunze@tu-ilmenau.de>
# @since    0.1.0
# @date     May, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Chiran Doschi, Christoph Dinh, Florian Schlembach, Daniel Knobl, Lorenz Esch, 
#                     Louis Eichhorst, Seok Lew, Tim Kunze. All rights reserved.
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
# @brief    This project file builds all applications.
#
#==============================================================================================================

include(../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    mne_rt_server \
    mne_forward_solution \
    mne_anonymize \
    mne_edf2fiff \

    qtHaveModule(charts) {

        SUBDIRS += \
            mne_dipole_fit \
            mne_scan \
            mne_analyze \
    } else {
        message("applications.pro - The Qt Charts module is missing. Please install to build the complete set of MNE-CPP features.")
    }

# Overwrite SUBDIRS if wasm flag was defined
contains(MNECPP_CONFIG, wasm) {
    SUBDIRS = mne_analyze \
              mne_anonymize \

    qtHaveModule(charts) {
        #SUBDIRS += # needs qt3D which is not yet wasm supported
    } else {
        message("applications.pro - The Qt Charts module is missing. Please install to build the complete set of MNE-CPP features.")
    }
}
