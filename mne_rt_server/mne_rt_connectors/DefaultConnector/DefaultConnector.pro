#-------------------------------------------------
#
# Project created by QtCreator 2012-01-23T19:30:27
#
#-------------------------------------------------

include(../../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += plugin

DEFINES += DEFAULTCONNECTOR_LIBRARY

#DEFINES += DACQ_OLD_CONNECTION_SCHEME # HP-UX

QT += network
QT -= gui

TARGET = DefaultConnector

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}

CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}RtCommunicationd
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}RtCommunication
}


unix:DESTDIR = $${PWD}/../../../bin/mne_rt_server_plugins

win32:DESTDIR = $${PWD}/../../../lib

win32 {
    FILE = $${DESTDIR}/$${TARGET}.dll
    PLUGINDIR = $${DESTDIR}/../bin/mne_rt_server_plugins
    FILE ~= s,/,\\,g
    PLUGINDIR ~= s,/,\\,g
    QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${PLUGINDIR}) $$escape_expand(\\n\\t)
}

SOURCES += ./*.cpp

HEADERS += ./*.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
