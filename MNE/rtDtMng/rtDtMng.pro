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

TEMPLATE = lib

QT += core gui widgets

DEFINES += RTDTMNG_LIBRARY

TARGET = RtDtMng
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}RtMeasd \
            -lMNE$${MNE_LIB_VERSION}Dispd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}RtMeas \
            -lMNE$${MNE_LIB_VERSION}Disp
}

DESTDIR = $${MNE_LIBRARY_DIR}

#
# win32: copy dll's to bin dir
# unix: add lib folder to LD_LIBRARY_PATH
#
win32 {
    FILE = $${DESTDIR}/$${TARGET}.dll
    BINDIR = $${DESTDIR}/../bin
    FILE ~= s,/,\\,g
    BINDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${BINDIR}) $$escape_expand(\\n\\t)
}

SOURCES += \
        rtmeasurementmanager.cpp

HEADERS += \
        rtmeasurementmanager.h\
        rtdtmng_global.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/disp

INSTALLS += header_files
