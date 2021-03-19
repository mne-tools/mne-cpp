#==============================================================================================================
#
# @file     libraries.pro
# @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
#           Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Lars Debor, Daniel Strohmeier, Lorenz Esch, Christoph Dinh. All rights reserved.
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
# @brief    This project file builds all MNE libraries.
#
#==============================================================================================================

include(../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    events \
    utils \
    fs \
    fiff \
    mne \
    fwd \
    inverse \
    communication \
    rtprocessing \
    connectivity \
    disp \

    qtHaveModule(charts) {
        # The Qt3D module is not yet Wasm supported
        !contains(MNECPP_CONFIG, wasm) {
            SUBDIRS += disp3D
        }
    } else {
        message("libraries.pro - The Qt Charts module is missing. Please install to build the disp3D library.")
    }

# Specify library dependencies
utils.depends =
fs.depends = utils
fiff.depends = utils
events.depends = utils
mne.depends = utils fs fiff
fwd.depends = utils fs fiff mne
inverse.depends = utils fs fiff mne fwd
communication.depends = utils fiff
rtprocessing.depends = utils connectivity fiff mne fwd inverse
connectivity.depends = utils fs fiff mne
disp.depends = utils fs fiff mne fwd inverse rtprocessing events
disp3D.depends = utils connectivity rtprocessing fs fiff mne fwd inverse disp
