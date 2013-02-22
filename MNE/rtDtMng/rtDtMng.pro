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
unix:CONFIG += static

DEFINES += RTDTMNG_LIBRARY

QT += core gui

CONFIG(debug, debug|release) {
    TARGET = rtdtmngd
    unix:DESTDIR = $$PWD/../../lib/unix/debug
    win32:DESTDIR = $$PWD/../../lib/win32/debug
    unix: LIBS += -L$$PWD/../../lib/unix/debug/ -lrtdispd
    win32:LIBS += -L$$PWD/../../lib/win32/debug/ -lrtdispd
    unix: LIBS += -L$$PWD/../../lib/unix/debug/ -lrtmeasd
    win32:LIBS += -L$$PWD/../../lib/win32/debug/ -lrtmeasd
    win32:QMAKE_POST_LINK += xcopy /y "..\\..\\..\\csa_rt\\lib\\win32\\debug\\rtdtmngd.dll" "..\\..\\..\\csa_rt\\bin\\win32\\debug\\"
}
else {
    TARGET = rtdtmng
    unix:DESTDIR = $$PWD/../../lib/unix/release
    win32:DESTDIR = $$PWD/../../lib/win32/release
    unix: LIBS += -L$$PWD/../../lib/unix/release/ -lrtdisp
    win32:LIBS += -L$$PWD/../../lib/win32/release/ -lrtdisp
    unix: LIBS += -L$$PWD/../../lib/unix/release/ -lrtmeas
    win32:LIBS += -L$$PWD/../../lib/win32/release/ -lrtmeas
    win32:QMAKE_POST_LINK += xcopy /y "..\\..\\..\\csa_rt\\lib\\win32\\release\\rtdtmng.dll" "..\\..\\..\\csa_rt\\bin\\win32\\release\\"
}

SOURCES += rtmeasurementmanager.cpp

HEADERS += rtmeasurementmanager.h\
        rtdtmng_global.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE62A4B50
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = rtdtmng.dll
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

symbian: LIBS += -lrtdisp

INCLUDEPATH += $$PWD/../rtdisp
DEPENDPATH += $$PWD/../rtdisp

symbian: LIBS += -lrtmeas

INCLUDEPATH += $$PWD/../rtmeas
DEPENDPATH += $$PWD/../rtmeas
