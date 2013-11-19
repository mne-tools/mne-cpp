#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_x.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     February, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Christoph Dinh, Limin Sun, Martin Luessi and Matti Hamalainen. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds the plugins for mne-x project.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    ecgsimulator \
    mnertclient \
    dummytoolbox \
    triggercontrol


#    rtsss \

win32 {
    exists(C:/Windows/System32/TMSiSDK.dll) {
        message(TMSI plugin configured! Driver TMSiSDK.dll found!)
        SUBDIRS += tmsi
    }
    else {
        message(TMSI plugin was not configured due to missing driver TMSiSDK.dll!)
        }
}
else {
    message(TMSI plugin was not configured due to wrong OS (win32 needed)!)
    }

contains(MNECPP_CONFIG, babyMEG) {
    message(BabyMEG plugin configured!)
#    SUBDIRS +=
}
else {
    message(RtServer plugin configured!)
    SUBDIRS += sourcelab

#    qtHaveModule(3d) {
#        message(Qt3D available: brainmonitor library configured!)
#        SUBDIRS += brainmonitor \
#    }
}

#    filtertoolbox \
#    gaborparticletoolbox \
#    megchannelsimulator \
#    megrtproc \
#    roitoolbox \
#    localizationtoolbox \
#    prelocalizationtoolbox


### BabyMEG alternative ###
#contains(MNECPP_CONFIG, babyMEG) {
#    message(BabyMEG plugin configured!)
#    SUBDIRS += babymeg
#}
#else {
#    message(RtServer plugin configured!)
#    SUBDIRS += rtclient
#}
