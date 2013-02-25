#--------------------------------------------------------
#
# Copyright (C) 2012 Christoph Dinh. All rights reserved.
#
# No part of this program may be photocopied, reproduced,
# or translated to another program language without the
# prior written consent of the author.
#
#--------------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += ECGSIMULATOR_LIBRARY

QT += core widgets

TARGET = ecgsimulator
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}Dispd \
            -lMNE$${MNE_LIB_VERSION}RtMeasd \
            -lMNE$${MNE_LIB_VERSION}RtDtMngd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}Disp \
            -lMNE$${MNE_LIB_VERSION}RtMeas \
            -lMNE$${MNE_LIB_VERSION}RtDtMng
}

DESTDIR = $${MNE_BINARY_DIR}/mne_x_plugins

SOURCES += \
        ecgsimulator.cpp \
        ecgsimchannel.cpp \
        ecgproducer.cpp \
        FormFiles/ecgsetupwidget.cpp \
        FormFiles/ecgrunwidget.cpp \
        FormFiles/ecgaboutwidget.cpp

HEADERS += \
        ecgsimulator.h\
        ecgsimulator_global.h \
        ecgsimchannel.h \
        ecgproducer.h \
        FormFiles/ecgsetupwidget.h \
        FormFiles/ecgrunwidget.h \
        FormFiles/ecgaboutwidget.h

FORMS += \
        FormFiles/ecgsetup.ui \
        FormFiles/ecgrun.ui \
        FormFiles/ecgabout.ui

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

OTHER_FILES += ecgsimulator.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}
