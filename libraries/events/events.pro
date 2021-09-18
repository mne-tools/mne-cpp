
include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += network concurrent
QT -= gui

DEFINES += EVENTS_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Events
TARGET = $$join(TARGET,,mnecpp,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppUtilsd \
} else {
    LIBS += -lmnecppUtils \
}

SOURCES += \
    event.cpp \
    eventgroup.cpp \
    eventmanager.cpp \
    events_global.cpp \
    eventsharedmemmanager.cpp

HEADERS += \
    event.h \
    eventgroup.h \
    events_global.h \
    eventmanager.h \
    eventsharedmemmanager.h

INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
}

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
}

################################################## BUILD TIMESTAMP/HASH UPDATER ############################################

FILE_TO_UPDATE = events_global.cpp
win32 {
    CONFIG(debug, debug|release) {
        OBJ_TARJET = debug\events_global.obj
    } else {
        OBJ_TARJET = release\events_global.obj
    }
}

ALL_FILES += $HEADERS
ALL_FILES += $SOURCES
ALL_FILES -= $FILE_TO_UPDATE

FileUpdater.target = phonyFileUpdater
for (I_FILE, ALL_FILES) {
    FileUpdater.depends += ${PWD}/${I_FILE}
}

unix|macx {
    FileUpdater.commands = touch ${PWD}/${FILE_TO_UPDATE} ; echo PASTA > phonyFileUpdater
}

win32 {
    FileUpdater.commands = copy /y $shell_path(${PWD})\${FILE_TO_UPDATE} +,, $shell_path(${PWD})\${FILE_TO_UPDATE} & echo PASTA > phonyFileUpdater
    OrderForcerTarget.target = ${OBJ_TARJET}
    OrderForcerTarget.depends += phonyFileUpdater
    QMAKE_EXTRA_TARGETS += OrderForcerTarget
}

PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdate
