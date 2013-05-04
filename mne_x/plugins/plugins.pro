#--------------------------------------------------------
#
# Copyright (C) 2012 Christoph Dinh. All rights reserved.
#
# No part of this program may be photocopied, reproduced,
# or translated to another program language without the
# prior written consent of the author.
#
#--------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = subdirs

SUBDIRS += \
    ecgsimulator \
    rtserver \
    dummytoolbox \
    sourcelab \
    rtsss \

#    filtertoolbox \
#    gaborparticletoolbox \
#    megchannelsimulator \
#    megrtproc \
#    roitoolbox \
#    localizationtoolbox \
#    prelocalizationtoolbox

qtHaveModule(3d) {
    message(Qt3D available: brainmonitor library configured!)
    SUBDIRS += brainmonitor \
}

### BabyMEG alternative ###
#contains(MNECPP_CONFIG, babyMEG) {
#    message(BabyMEG plugin configured!)
#    SUBDIRS += babymeg
#}
#else {
#    message(RtServer plugin configured!)
#    SUBDIRS += rtserver
#}
