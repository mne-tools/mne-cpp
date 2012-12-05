#-------------------------------------------------
#
# Project created by QtCreator 2012-12-04T22:32:33
#
#-------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT       -= gui

DEFINES += FWDRT_LIBRARY

TARGET = fwdrt

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${PWD}/../../lib/
CONFIG(debug, debug|release) {
    LIBS += -lfiffd
    LIBS += -lmned
}
else {
    LIBS += -lfiff
    LIBS += -lmne
}


DESTDIR = $${PWD}/../../lib

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

SOURCES += fwdrt.cpp

HEADERS += fwdrt.h\
        fwdrt_global.h
