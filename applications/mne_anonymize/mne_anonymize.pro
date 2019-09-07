#--------------------------------------------------------------------------------------------------------------
#
<<<<<<< HEAD
# @file     mne_anonymize.pro
=======
# @file     mne_fiff_anonymizer.pro
>>>>>>> creating fiffanonymizer class and main.cpp
# @author   Juan Garcia-Prieto <Juan.GarciaPrieto@uth.tmc.edu> <juangpc@gmail.com>;
#           Wayne Mead <wayne.mead@uth.tmc.edu> <wayne.isk@gmail.com>;
#           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           John C. Mosher <John.C.Mosher@uth.tmc.edu> <jcmosher@gmail.com>;
# @version  1.0
# @date     August, 2019
#
# @section  LICENSE
#
# Copyright (C) 2019, Juan Garcia-Prieto and Matti Hamalainen. All rights reserved.
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
# @brief    This project file generates the makefile to build the mne_fiff_anonymizer example.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT += widgets concurrent
CONFIG   += console
CONFIG   -= app_bundle

TARGET = mne_anonymize

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fiff \
}

DESTDIR = $${MNE_BINARY_DIR}

SOURCES += main.cpp \
    fiffanonymizer.cpp \
    settingscontroller.cpp \

HEADERS  += fiffanonymizer.h \
        settingscontroller.h \

FORMS    +=

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_FIFF_ANONYMIZER_DIR}

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $$PWD
