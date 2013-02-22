#--------------------------------------------------------
#
# Copyright (C) 2012 Christoph Dinh. All rights reserved.
#
# No part of this program may be photocopied, reproduced,
# or translated to another program language without the
# prior written consent of the author.
#
#--------------------------------------------------------

TEMPLATE = lib

CONFIG += plugin

DEFINES += ECGSIMULATOR_LIBRARY

QT += core gui

CONFIG(debug, debug|release) {
    TARGET = ecgsimulatord
    unix:DESTDIR = $$PWD/../../bin/unix/debug/modules
    win32:DESTDIR = $$PWD/../../bin/win32/debug/modules
    unix: LIBS += -L$$PWD/../../lib/unix/debug/ -lrtmeasd
    #unix: PRE_TARGETDEPS += $$OUT_PWD/../../comp/rtmeas/librtmeasd.a
    win32:LIBS += -L$$PWD/../../lib/win32/debug/ -lrtmeasd
    win32:PRE_TARGETDEPS += $$PWD/../../lib/win32/debug/rtmeasd.lib
}
else {
    TARGET = ecgsimulator
    unix:DESTDIR = $$PWD/../../bin/unix/release/modules
    win32:DESTDIR = $$PWD/../../bin/win32/release/modules
    unix: LIBS += -L$$PWD/../../lib/unix/release/ -lrtmeas
    #unix: PRE_TARGETDEPS += $$OUT_PWD/../../comp/rtmeas/librtmeas.a
    win32:LIBS += -L$$PWD/../../lib/win32/release/ -lrtmeas
    win32:PRE_TARGETDEPS += $$PWD/../../lib/win32/release/rtmeas.lib
}

SOURCES += ecgsimulator.cpp \
    ecgsimchannel.cpp \
    ecgproducer.cpp \
    FormFiles/ecgsetupwidget.cpp \
    FormFiles/ecgrunwidget.cpp \
    FormFiles/ecgaboutwidget.cpp

HEADERS += ecgsimulator.h\
        ecgsimulator_global.h \
    ecgsimchannel.h \
    ecgproducer.h \
    FormFiles/ecgsetupwidget.h \
    FormFiles/ecgrunwidget.h \
    FormFiles/ecgaboutwidget.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE28783FA
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = ecgsimulator.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

FORMS += \
    FormFiles/ecgsetup.ui \
    FormFiles/ecgrun.ui \
    FormFiles/ecgabout.ui

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $$PWD

symbian: LIBS += -lrtmeas

INCLUDEPATH += $$PWD/../../comp/rtmeas
DEPENDPATH += $$PWD/../../comp/rtmeas
