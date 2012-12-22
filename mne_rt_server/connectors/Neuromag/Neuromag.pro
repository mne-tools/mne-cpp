#-------------------------------------------------
#
# Project created by QtCreator 2012-01-23T19:30:27
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG += plugin

DEFINES += NEUROMAG_LIBRARY

#DEFINES += DACQ_OLD_CONNECTION_SCHEME # HP-UX

QT += network
QT -= gui

TARGET = Neuromag

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${PWD}/../../../lib/

CONFIG(debug, debug|release) {
    LIBS += -lmned
    LIBS += -lfiffd
    LIBS += -lgenericsd
}
else {
    LIBS += -lmne
    LIBS += -lfiff
    LIBS += -lgenerics
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




SOURCES += neuromag.cpp \
    dacqserver.cpp \
    collectorsocket.cpp \
    shmemsocket.cpp

HEADERS += neuromag.h\
    neuromag_global.h \
    ../../mne_rt_server/IConnector.h \  #IConnector is a Q_OBJECT and the resulting moc file needs to be known -> that's why inclution is important!
    types_definitions.h \
    dacqserver.h \
    collectorsocket.h \
    shmemsocket.h

INCLUDEPATH += $${PWD}/../../../include/3rdParty

OTHER_FILES += neuromag.json

# Put generated form headers into the origin --> cause other src is pointing at them
UI_DIR = $${PWD}








#  The QML_INFRA_FILES and QML_MESHES_FILES are both about QML based
# applications, so we'll install them into QT_INSTALL_DATA instead of
# QT_INSTALL_BINS
# QML_INFRA_FILES is used by our quick3d demos and examples to indicate files
# that are part of the application and should be installed (e.g. qml files,
# images, meshes etc).
# This conditional serves two purposes:
# 1) Set up a qmake extra compiler to copy relevant QML files at build time
#    to allow for a normal "change, make, test" developement cycle
# 2) Set up appropriate install paths on the same files to use "make install"
#    for building packages
#!isEmpty(QML_INFRA_FILES) {

#    # rules to copy files from the *base level* of $$PWD/qml into the right place
#    copyqmlinfra_install.files = $$QML_INFRA_FILES
#    copyqmlinfra_install.path = $$target.path/$$resource_dir/qml
#    INSTALLS += copyqmlinfra_install

#    # put all our demos/examples and supporting files into $BUILD_DIR/bin
#    target_dir = $$DESTDIR/$$resource_dir/qml
#    # create extra qmake compiler to copy files across during build step
#    copyqmlinfra.input = QML_INFRA_FILES
#    copyqmlinfra.output = $$target_dir/${QMAKE_FILE_IN_BASE}${QMAKE_FILE_EXT}
#    copyqmlinfra.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
#    copyqmlinfra.CONFIG += no_link no_clean
#    copyqmlinfra.variable_out = POST_TARGETDEPS
#    QMAKE_EXTRA_COMPILERS += copyqmlinfra
#}
