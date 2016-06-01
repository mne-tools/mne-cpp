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
# @brief    This project file builds the plugins for mne-x project.
#
#--------------------------------------------------------------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = subdirs

contains(MNECPP_CONFIG, BuildBasicMNEXVersion) {
    message(Building basic MNE-X version!)
    #Sensors
    SUBDIRS += \
        babymeg \
        fiffsimulator \

    #Algorithms
    SUBDIRS += \
} else {
    #Sensors
    SUBDIRS += \
        ecgsimulator \
        fiffsimulator \
        neuromag \
        babymeg \
        triggercontrol \
        #gusbamp \
        #eegosports

    #Algorithms
    SUBDIRS += \
        dummytoolbox \
        mne \
        rapmusictoolbox \
        averaging \
        covariance \
        noise \
        # bci \
        rtsss \
        rthpi \
        noisereduction

    win32 { #Only compile the TMSI plugin if a windows system is used - TMSi driver is not available for linux yet
        contains(QMAKE_HOST.arch, x86_64) { #Compiling MNE-X FOR a 64bit system
            exists(C:/Windows/System32/TMSiSDK.dll) {
                message(Compiling MNE-X FOR a 64bit system: TMSI plugin configured! TMSi Driver found!)
                SUBDIRS += tmsi
            }
        }
        else {
            exists(C:/Windows/SysWOW64/TMSiSDK32bit.dll) { #Compiling MNE-X FOR a 32bit system ON a 64bit system
                message(Compiling MNE-X FOR a 32bit system ON a 64bit system: TMSI plugin configured! TMSi Driver found!)
                SUBDIRS += tmsi
            }
            else {
                exists(C:/Windows/System32/TMSiSDK.dll) { #Compiling MNE-X FOR a 32bit system ON a 32bit system
                    message(Compiling MNE-X FOR a 32bit system ON a 32bit system: TMSI plugin configured! TMSi Driver found!)
                    SUBDIRS += tmsi
                }
                else {
                    message(TMSI plugin not configured! TMSi Driver not found!)
                }
            }
        }
    }
    else {
        message(TMSI plugin was not configured due to wrong OS (win32 needed)!)
    }
}
