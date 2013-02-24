#--------------------------------------------------------------------------------------------------------------
#
# @file     mne_x.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     February, 2013
#
# @section  LICENSE
#
# Copyright (C) 2013, Christoph Dinh, Martin Luessi and Matti Hamalainen. All rights reserved.
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
# @brief    This project file builds the mne-x main application.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = app

QT += network core gui widgets

TARGET = mne_x

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
}

DESTDIR = $${PWD}/../../bin

SOURCES += \
    src/main.cpp \
    src/FormFiles/startupwidget.cpp \
    src/FormFiles/runwidget.cpp \
    src/FormFiles/moduledockwidget.cpp \
    src/FormFiles/mainwindow.cpp \
    src/FormFiles/mainsplashscreen.cpp \
    src/management/modulemanager.cpp \
    src/management/connector.cpp

HEADERS += \
    src/FormFiles/startupwidget.h \
    src/FormFiles/runwidget.h \
    src/FormFiles/moduledockwidget.h \
    src/FormFiles/mainwindow.h \
    src/FormFiles/mainsplashscreen.h \
    src/interfaces/ISensor.h \
    src/interfaces/IRTVisualization.h \
    src/interfaces/IRTRecord.h \
    src/interfaces/IRTAlgorithm.h \
    src/interfaces/IModule.h \
    src/interfaces/IAlert.h \
    src/management/modulemanager.h \
    src/management/connector.h \
    src/preferences/info.h

FORMS   +=

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

RESOURCES += \
    res/mainApp.qrc

OTHER_FILES += \
    res/QtStyleSheets/plastique.qss \
    res/images/splashscreen/splashscreen_csa_rt.png \
    res/images/icons/zoomStd.png \
    res/images/icons/zoomOut.png \
    res/images/icons/zoomIn.png \
    res/images/icons/visualisation.png \
    res/images/icons/stop.png \
    res/images/icons/sensor.png \
    res/images/icons/save.png \
    res/images/icons/run.png \
    res/images/icons/new.png \
    res/images/icons/displayMax.png \
    res/images/icons/csa_rt.png \
    res/images/icons/algorithm.png
